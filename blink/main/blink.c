#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO 16 // 使用するGPIOピン

void app_main(void) {
  // GPIOの設定
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

  while (1) {
    gpio_set_level(LED_GPIO, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(LED_GPIO, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
