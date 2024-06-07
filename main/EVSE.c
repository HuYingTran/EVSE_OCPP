// HTTP Client - FreeRTOS ESP IDF - GET

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

typedef enum {
    STATE_READY,
    STATE_CONNECTED,
    STATE_CHARGING,
    STATE_ERROR
} state_t;

state_t current_state = STATE_READY;

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
                printf("led 2: %d \n", LED2_PIN);
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
}