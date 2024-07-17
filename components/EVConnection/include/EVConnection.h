#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include "esp_event.h"

#define CONFIG_LED_PIN 2
#define ESP_INTR_FLAG_DEFAULT 0
#define CONFIG_BUTTON_PIN 0

bool isConnected;

TaskHandle_t ISR;

SemaphoreHandle_t xSemaphoreHTTP;

void IRAM_ATTR button_isr_handler(void* arg);

void button_task(void *arg);

bool check_ev_connected();

void connect_checking_init(void);