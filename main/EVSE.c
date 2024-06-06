// HTTP Client - FreeRTOS ESP IDF - GET

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "my_data.h"
#include "cJSON.h"
#include "driver/gpio.h"

#define BUTTON_PIN  GPIO_NUM_0   // GPIO input cho nút bấm
#define LED_PIN     GPIO_NUM_2   // GPIO output cho LED màu xanh
#define RELAY_PIN   GPIO_NUM_14   // GPIO output cho relay

#define LED_ON      1
#define LED_OFF     0

#define BUTTON_PRESSED  0
#define BUTTON_RELEASED 1

#define BLINK_DELAY     500  // Độ trễ giữa các lần nhấp nháy LED (ms)

typedef enum {
    STATE_READY,
    STATE_CONNECTED,
    STATE_CHARGING,
    STATE_ERROR
} state_t;

state_t current_state = STATE_READY;
uint8_t relay_state = 0;

void button_task(void *pvParameter) {
    gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

    while(1) {
        // Đọc trạng thái của nút bấm
        int button_state = gpio_get_level(BUTTON_PIN);

        if (button_state == BUTTON_PRESSED) {
            // Nếu nút được nhấn
            if (current_state == STATE_READY) {
                current_state = STATE_CONNECTED;
            } 
            if (relay_state == 1 && current_state == STATE_CONNECTED) {
                current_state = STATE_CHARGING;
            }
        } else {
            // Nếu nút được nhả ra
            if (current_state == STATE_CHARGING) {
                current_state = STATE_READY;
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);  // Đợi 100ms tránh đọc nhầm
    }
}

void led_task(void *pvParameter) {
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        switch(current_state) {
            case STATE_READY:
                gpio_set_level(LED_PIN, LED_OFF);
                break;
            case STATE_CONNECTED:
                gpio_set_level(LED_PIN, LED_ON);
                vTaskDelay(BLINK_DELAY / portTICK_PERIOD_MS);
                gpio_set_level(LED_PIN, LED_OFF);
                vTaskDelay(BLINK_DELAY / portTICK_PERIOD_MS);
                break;
            case STATE_CHARGING:
                gpio_set_level(LED_PIN, LED_ON);
                break;
            case STATE_ERROR:
                // Nhấp nháy LED để biểu thị lỗi
                gpio_set_level(LED_PIN, LED_ON);
                vTaskDelay(BLINK_DELAY / portTICK_PERIOD_MS);
                gpio_set_level(LED_PIN, LED_OFF);
                vTaskDelay(BLINK_DELAY / portTICK_PERIOD_MS);
                break;
            default:
                break;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Đợi 100ms tránh nhấp nháy quá nhanh
    }
}

void relay_task(void *pvParameter) {
    gpio_pad_select_gpio(RELAY_PIN);
    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        if (current_state == STATE_CHARGING) {
            gpio_set_level(RELAY_PIN, LED_ON);
        } else {
            gpio_set_level(RELAY_PIN, LED_OFF);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Đợi 100ms trước khi kiểm tra lại trạng thái

        printf("\n Current state: %d", current_state);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}


static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;
    default:
		printf("%d ", event_id);
        break;
    }
}

void wifi_connection()
{
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation 					s1.1
    esp_event_loop_create_default();     // event loop 			                s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station 	                    s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); // 					                    s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS}};

	esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
 
    esp_wifi_start();

    esp_wifi_connect();
}

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    cJSON *root = NULL;
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:

        root = cJSON_Parse((char *)evt->data);
            
        if(root == NULL){
            printf("ERROR parsing JSON");
            return ESP_FAIL;
        }

        cJSON *feeds = cJSON_GetObjectItem(root, "feeds");
        if (feeds != NULL && cJSON_IsArray(feeds))
        {
            cJSON *feed = cJSON_GetArrayItem(feeds, 1); // Lấy phần tử đầu tiên trong mảng feeds
            if (feed != NULL)
            {
                cJSON *field1 = cJSON_GetObjectItem(feed, "field1");
                if (field1 != NULL)
                {
                    if (cJSON_IsString(field1))
                    {
                        relay_state = strcmp(field1->valuestring, "1") == 0 ? 1 : 0;
                    }
                    }
                }
            }
        
        else
        {
            printf("feeds array not found or is not an array\n");
        }
        break;
    default:
        break;
    }
    cJSON_Delete(root);
    return ESP_OK;
}


static void rest_get()
{
    esp_http_client_config_t config_get = {
        .url = "http://api.thingspeak.com/channels/2510901/feeds.json?results=2",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void app_main(void)
{
    
    printf("WIFI was initiated ...........\n\n");
    nvs_flash_init();
    wifi_connection();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);
    xTaskCreate(relay_task, "relay_task", 2048, NULL, 10, NULL);

    while (1) {

        rest_get();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}