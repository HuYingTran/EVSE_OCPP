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

#include "lwip/err.h"
#include "lwip/sys.h"

/* MicroOcpp includes */
#include <mongoose.h>
#include <MicroOcpp_c.h> //C-facade of MicroOcpp
#include <MicroOcppMongooseClient_c.h> //WebSocket integration for ESP-IDF



#define EXAMPLE_MO_OCPP_BACKEND    CONFIG_MO_OCPP_BACKEND
#define EXAMPLE_MO_CHARGEBOXID     CONFIG_MO_CHARGEBOXID
#define EXAMPLE_MO_AUTHORIZATIONKEY CONFIG_MO_AUTHORIZATIONKEY

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    /* Initialize Mongoose (necessary for MicroOcpp)*/
    struct mg_mgr mgr;        // Event manager
    mg_mgr_init(&mgr);        // Initialise event manager
    mg_log_set(MG_LL_DEBUG);  // Set log level

    /* Initialize MicroOcpp */
    struct OCPP_FilesystemOpt fsopt = { .use = true, .mount = true, .formatFsOnFail = true};

    OCPP_Connection *osock = ocpp_makeConnection(&mgr,
            EXAMPLE_MO_OCPP_BACKEND, 
            EXAMPLE_MO_CHARGEBOXID, 
            EXAMPLE_MO_AUTHORIZATIONKEY, "", fsopt);
    ocpp_initialize(osock, "ESP-IDF charger", "Your brand name here", fsopt, false);

    connect_checking_init();
    ocpp_setConnectorPluggedInput(check_ev_connected);

    //Create and start stats task
    xTaskCreate( button_task, "button_task", 4096, NULL , 10, &ISR );

    /* Enter infinite loop */
    while (1) {
        mg_mgr_poll(&mgr, 10);
        ocpp_loop();
    }
    
    /* Deallocate ressources */
    ocpp_deinitialize();
    ocpp_deinitConnection(osock);
    mg_mgr_free(&mgr);
    return;
}
