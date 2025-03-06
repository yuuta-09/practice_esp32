#include <stdio.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define IR_PIN GPIO_NUM_4
#define MAX_PULSES 100

typedef struct {
  uint32_t duration;
  bool level;
} ir_pulse_t;

static ir_pulse_t pulses[MAX_PULSES];
static int pulse_count = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
  static uint64_t last_time = 0;
  uint64_t current_time = esp_timer_get_time();

  if (pulse_count < MAX_PULSES) {
    pulses[pulse_count].duration = current_time - last_time;
    pulses[pulse_count].level = gpio_get_level(IR_PIN);
    pulse_count++;
  }

  last_time = current_time;
}

void app_main(void) {
  // GPIO設定
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << IR_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE,
  };
  gpio_config(&io_conf);

  // 割り込み設定
  gpio_install_isr_service(0);
  gpio_isr_handler_add(IR_PIN, gpio_isr_handler, NULL);

  while (1) {
    if (pulse_count > 0) {
      ESP_LOGI("IR_RECEIVER", "Received IR signal:");
      for (int i = 0; i < pulse_count; i++) {
        ESP_LOGI("IR_RECEIVER", "Pulse %d: Duration=%lu us, Level=%d", i,
                 pulses[i].duration, pulses[i].level);
      }
      pulse_count = 0; // リセット
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
