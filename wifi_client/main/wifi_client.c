#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_station";

/* イベントグループのビット定義 */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
#define MAXIMUM_RETRY 5

/* イベントハンドラ: Wi-FiおよびIPイベントのコールバック */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_LOGI(TAG, "WiFi started, attempting to connect...");
      esp_wifi_connect();
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      // 最大リトライ回数まで接続を試みる
      if (s_retry_num < MAXIMUM_RETRY) {
        esp_wifi_connect();
        s_retry_num++;
        ESP_LOGI(TAG, "Retry to connect to the AP(%d %d)", s_retry_num,
                 MAXIMUM_RETRY);
      } else {
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
      }
      ESP_LOGI(TAG, "Connection to the AP failed");
      break;
    default:
      break;
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "GOT IP: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

// Wi-Fi STAモードの初期化
void wifi_init_sta(void) {
  /* イベントグループの作成 */
  s_wifi_event_group = xEventGroupCreate();

  /* TCP/IP スタックの初期化 */
  ESP_ERROR_CHECK(esp_netif_init());

  /* デフォルトループ作成 */
  ESP_ERROR_CHECK(esp_event_loop_create_default());
}
