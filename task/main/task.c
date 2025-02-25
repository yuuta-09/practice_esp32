#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"

#define LED_GPIO GPIO_NUM_16
#define BOOT_BUTTON GPIO_NUM_0

// LEDを1秒ごとに点滅させるタスク
void blink_led_task(void *pvParameter) {
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

  while (1) {
    gpio_set_level(LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    gpio_set_level(LED_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// BOOTボタンを監視するタスク
void button_monitor_task(void *pvParameter) {
  gpio_reset_pin(BOOT_BUTTON);
  gpio_set_direction(BOOT_BUTTON, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BOOT_BUTTON, GPIO_PULLUP_ONLY);

  while (1) {
    if (gpio_get_level(BOOT_BUTTON) == 0) {
      ESP_LOGI("BUTTON", "BOOT button pressed!");
      vTaskDelay(pdMS_TO_TICKS(500)); // チャタリング防止
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void app_main(void) {
  // LED点滅タスクの作成
  xTaskCreate(blink_led_task, "Blink LED", 2048, NULL, 2, NULL);

  // BOOTボタンを監視するタスク
  xTaskCreate(button_monitor_task, "Button Monitor", 2048, NULL, 2, NULL);
}
