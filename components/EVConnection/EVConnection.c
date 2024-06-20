#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include "EVConnection.h"

bool isConnected = false;

TaskHandle_t ISR = NULL;

// interrupt service routine, called when the button is pressed
void IRAM_ATTR button_isr_handler(void* arg) {
  
xTaskResumeFromISR(ISR);
//portYIELD_FROM_ISR(  );
}

// task that will react to button clicks
void button_task(void *arg)
{
    bool led_status = false;
    while(1){  
        vTaskSuspend(NULL);
        isConnected = !isConnected;
        led_status = !led_status;
        gpio_set_level(CONFIG_LED_PIN, led_status);
    }  
}

bool check_ev_connected() {
    return isConnected;
}

void connect_checking_init(void)
{
    gpio_pad_select_gpio(CONFIG_BUTTON_PIN);
    gpio_pad_select_gpio(CONFIG_LED_PIN);
  
    // set the correct direction
    gpio_set_direction(CONFIG_BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(CONFIG_LED_PIN, GPIO_MODE_OUTPUT);
  
    // enable interrupt on falling (1->0) edge for button pin
    gpio_set_intr_type(CONFIG_BUTTON_PIN, GPIO_INTR_NEGEDGE);

  
    //Install the driver’s GPIO ISR handler service, which allows per-pin GPIO interrupt handlers.
    // install ISR service with default configuration
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  
    // attach the interrupt service routine
    gpio_isr_handler_add(CONFIG_BUTTON_PIN, button_isr_handler, NULL);
}
