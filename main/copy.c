#include <string.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "WIFI.h"
#include "EVConnection.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* MicroOcpp includes */
#include <mongoose.h>
#include <MicroOcpp.h> //C-facade of MicroOcpp
#include <MicroOcppMongooseClient_c.h> //WebSocket integration for ESP-IDF

#define EXAMPLE_MO_OCPP_BACKEND         CONFIG_MO_OCPP_BACKEND
#define EXAMPLE_MO_CHARGEBOXID          CONFIG_MO_CHARGEBOXID
#define EXAMPLE_MO_AUTHORIZATIONKEY     CONFIG_MO_AUTHORIZATIONKEY

#define RELAY_PIN                       GPIO_NUM_14

struct mg_mgr       mgr;      //event manager
static uint8_t      mode = 0;
 TickType_t         LastWakeTime = 0;
 TaskHandle_t       get_task_handler = NULL;

 void relay_task(void *pvParameter) {
    gpio_pad_select_gpio(RELAY_PIN);
 }