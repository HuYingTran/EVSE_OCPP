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

#define BUTTON1_PIN     GPIO_NUM_0   // GPIO input cho nút bấm
#define BUTTON2_PIN     GPIO_NUM_26
#define LED1_PIN        GPIO_NUM_2 
#define LED2_PIN        GPIO_NUM_33
#define RELAY_PIN       GPIO_NUM_14   // GPIO output cho relay

#define LED_ON      1
#define LED_OFF     0

#define BUTTON_PRESSED  0
#define BUTTON_RELEASED 1

#define BLINK_DELAY     500  // Độ trễ giữa các lần nhấp nháy LED (ms)

// #define FIREBASE_URL "http://evse-abc20-default-rtdb.firebaseio.com/mode.json" // Endpoint cho node duy nhất
// #define FIREBASE_TOKEN ""  // Token xác thực (nếu cần)

typedef enum {
    STATE_READY,
    STATE_CONNECTED,
    STATE_CHARGING,
    STATE_ERROR
} state_t;

state_t current_state = STATE_READY;
static uint8_t mode = 0;

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
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
}

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        // Phân tích dữ liệu JSON nếu cần
        cJSON *root = cJSON_Parse(evt->data);
        if (root != NULL)
        {
            // Ví dụ: Lấy một trường cụ thể từ JSON
            cJSON *field = cJSON_GetObjectItem(root, "field1");
            if (cJSON_IsString(field))
            {
                mode = strcmp(field->valuestring, "5") == 0 ? 1 : 0;
            }
            cJSON_Delete(root);
        }
    }
    return ESP_OK;
}

static void rest_get()
{

    esp_http_client_config_t config_get = {
        .url = "http://evse-abc20-default-rtdb.firebaseio.com/.json",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void button_task(void *pvParameter) {
    gpio_pad_select_gpio(BUTTON1_PIN);
    gpio_set_direction(BUTTON1_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON1_PIN, GPIO_PULLUP_ONLY);

    gpio_pad_select_gpio(BUTTON2_PIN);
    gpio_set_direction(BUTTON2_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON2_PIN, GPIO_PULLUP_ONLY);

    while(1) {
        // Đọc trạng thái của nút bấm
        int button1_state = gpio_get_level(BUTTON1_PIN);
        int button2_state = gpio_get_level(BUTTON2_PIN);
        

        if (button1_state == BUTTON_PRESSED) {
            // Nếu nút 1 được nhấn
            if (current_state == STATE_READY) {
                current_state = STATE_CONNECTED;
            } 
            // Nếu nút 2 được nhấn
            if (button2_state == BUTTON_PRESSED) {
                current_state = STATE_CHARGING;
            }
            else {
                current_state = STATE_CONNECTED;
            }
        } 
        else {
            // Nếu nút được nhả ra
            current_state = STATE_READY;
            }
        

        vTaskDelay(100 / portTICK_PERIOD_MS);  // Đợi 100ms tránh đọc nhầm
        printf("button 2: %d \n", button2_state);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void led_task(void *pvParameter) {
    gpio_pad_select_gpio(LED1_PIN);
    gpio_set_direction(LED1_PIN, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED2_PIN);
    gpio_set_direction(LED2_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        switch(current_state) {
            case STATE_READY:
                gpio_set_level(LED1_PIN, LED_OFF);
                gpio_set_level(LED2_PIN, LED_OFF);
                break;
            case STATE_CONNECTED:
                gpio_set_level(LED1_PIN, LED_ON);
                gpio_set_level(LED2_PIN, LED_OFF);
                break;
            case STATE_CHARGING:
                gpio_set_level(LED1_PIN, LED_ON);
                gpio_set_level(LED2_PIN, LED_ON);
                break;
            case STATE_ERROR:
                // Nhấp nháy LED để biểu thị lỗi
                gpio_set_level(LED1_PIN, LED_ON);
                gpio_set_level(LED2_PIN, LED_ON);
                vTaskDelay(BLINK_DELAY / portTICK_PERIOD_MS);
                gpio_set_level(LED1_PIN, LED_OFF);
                gpio_set_level(LED2_PIN, LED_OFF);
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
            gpio_set_level(RELAY_PIN, 1);
        } else {
            gpio_set_level(RELAY_PIN, 0);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Đợi 100ms trước khi kiểm tra lại trạng thái

        printf("\n Current state: %d \n", current_state);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);
    xTaskCreate(relay_task, "relay_task", 2048, NULL, 10, NULL);
    printf("WIFI was initiated ...........\n\n");
    nvs_flash_init();
    wifi_connection();
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    while (1) {
        rest_get();
        printf("mode: %d", mode);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}