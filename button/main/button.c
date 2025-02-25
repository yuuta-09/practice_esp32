#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

#define BOOT_BUTTON GPIO_NUM_0
#define LED_PIN GPIO_NUM_16

void app_main(void) {
  // GPIOのリセット
  gpio_reset_pin(BOOT_BUTTON);
  gpio_reset_pin(LED_PIN);

  // BootボタンのGPIOの初期化
  gpio_set_direction(BOOT_BUTTON, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BOOT_BUTTON, GPIO_PULLUP_ONLY);

  // LEDのGPIOの初期化
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

  while (1) {
    int button_state = gpio_get_level(BOOT_BUTTON);

    if (button_state == 0) {
      gpio_set_level(LED_PIN, 1);
    } else {
      gpio_set_level(LED_PIN, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
