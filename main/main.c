/* Based on the ESP-IDF WiFi station Example (see https://github.com/espressif/esp-idf/tree/release/v4.4/examples/wifi/getting_started/station/main)

   This example code extends the WiFi example with the necessary calls to establish an
   OCPP connection on the ESP-IDF. 
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <driver/gpio.h>
#include "WIFI.h"
#include "EVConnection.h"
#include "esp_http_client.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* MicroOcpp includes */
#include <mongoose.h>
#include <MicroOcpp_c.h> //C-facade of MicroOcpp
#include <MicroOcppMongooseClient_c.h> //WebSocket integration for ESP-IDF



#define EXAMPLE_MO_OCPP_BACKEND    CONFIG_MO_OCPP_BACKEND
#define EXAMPLE_MO_CHARGEBOXID     CONFIG_MO_CHARGEBOXID
#define EXAMPLE_MO_AUTHORIZATIONKEY CONFIG_MO_AUTHORIZATIONKEY

#define RELAY_PIN       GPIO_NUM_14

struct mg_mgr mgr;        // Event manager
static uint8_t mode = 0;
SemaphoreHandle_t xSemaphoreHTTP;
// SemaphoreHandle_t xSemaphoreOCPP;

void relay_task(void *pvParameter) {
    gpio_pad_select_gpio(RELAY_PIN);
    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        gpio_set_level(RELAY_PIN, mode == 1 ? 1 : 0);
        vTaskDelay(100 / portTICK_PERIOD_MS); // Đợi 100ms trước khi kiểm tra lại trạng thái
    }
}
esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id){
        case HTTP_EVENT_ON_DATA:
        if (evt->data_len >0) {
            cJSON *root = cJSON_Parse((char *)evt->data);
            if (root != NULL){
                cJSON *mode_item = cJSON_GetObjectItem(root, "Mode");
                if (cJSON_IsNumber(mode_item)) {
                    mode = mode_item->valueint;
                    printf("Mode: %d\n", mode);
                }
                cJSON_Delete(root);
            } else {
                printf("Failed to parse JSON\n");
            }
        } else {
            printf("data_len = 0");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void rest_get_task(void *pvParameter)
{
    while(1) {
        printf("\nATTEMPTING TO TAKE XSEMAPHOREHTTP FOR HTTP REQUEST\n");
        if (xSemaphoreTake(xSemaphoreHTTP, portMAX_DELAY)) {
            printf("XSEMAPHOREHTTP TAKEN FOR HTTP REQUEST\n");
            esp_http_client_config_t config_get = {
                .url = "https://evse-b1229-default-rtdb.firebaseio.com/.json",
                .method = HTTP_METHOD_GET,
                .cert_pem = NULL,
                .event_handler = client_event_get_handler,
                .skip_cert_common_name_check = true,
                .timeout_ms = 1000
            } ;
            esp_http_client_handle_t client = esp_http_client_init(&config_get);
            esp_http_client_perform(client);
            esp_http_client_cleanup(client);

            xSemaphoreGive(xSemaphoreHTTP);
            printf("XSEMAPHOREHTTP RELEASED AFTER HTTP REQUEST\n");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void ocpp_task(void *pvParameter)
{
    while(1) {
        printf("\nAttempting to take xSemaphoreOCPP loop\n");
        if (xSemaphoreTake(xSemaphoreHTTP, portMAX_DELAY)) {
            printf("xSemaphoreOCPP taken for OCPP loop\n");
            mg_mgr_poll(&mgr, 10);
            ocpp_loop();
            xSemaphoreGive(xSemaphoreHTTP);
            printf("xSemaphoreOCPP released after OCPP loop\n");
        } else {
            printf("Failed to take OCPP semaphore");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    xSemaphoreHTTP = xSemaphoreCreateMutex();
    // xSemaphoreOCPP = xSemaphoreCreateMutex();
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    /* Initialize Mongoose (necessary for MicroOcpp)*/
    mg_mgr_init(&mgr);        // Initialise event manager
    mg_log_set(MG_LL_DEBUG);  // Set log level

    /* Initialize MicroOcpp */
    struct OCPP_FilesystemOpt fsopt = { .use = true, .mount = true, .formatFsOnFail = true};

    OCPP_Connection *osock = ocpp_makeConnection(&mgr,
            EXAMPLE_MO_OCPP_BACKEND, 
            EXAMPLE_MO_CHARGEBOXID, 
            EXAMPLE_MO_AUTHORIZATIONKEY, "", fsopt);
    ocpp_initialize(osock, "cc:7b:5c:27:09:70", "ESP32", fsopt, false);

    connect_checking_init();
    ocpp_setConnectorPluggedInput(check_ev_connected);

    //Create and start stats task
    xTaskCreate(button_task, "button_task", 4096, NULL , 10, &ISR);
    xTaskCreate(relay_task, "relay_task", 4096, NULL , 10, NULL);
    xTaskCreate(rest_get_task, "rest_get_task", 4096, NULL, 10, NULL);
    xTaskCreate(ocpp_task, "ocpp_task", 4096, NULL, 10, NULL);
    return;
}
