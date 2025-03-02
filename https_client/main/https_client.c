#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

const char *server_cert_pem =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDeTCCAv+gAwIBAgIQCwDpLU1tcx/KMFnHyx4YhjAKBggqhkjOPQQDAzBhMQsw\n"
    "CQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3d3cu\n"
    "ZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBHMzAe\n"
    "Fw0yMTA0MTQwMDAwMDBaFw0zMTA0MTMyMzU5NTlaMFkxCzAJBgNVBAYTAlVTMRUw\n"
    "EwYDVQQKEwxEaWdpQ2VydCBJbmMxMzAxBgNVBAMTKkRpZ2lDZXJ0IEdsb2JhbCBH\n"
    "MyBUTFMgRUNDIFNIQTM4NCAyMDIwIENBMTB2MBAGByqGSM49AgEGBSuBBAAiA2IA\n"
    "BHipnHWuiF1jpK1dhtgQSdavklljQyOF9EhlMM1KNJWmDj7ZfAjXVwUoSJ4Lq+vC\n"
    "05ae7UXSi4rOAUsXQ+Fzz21zSDTcAEYJtVZUyV96xxMH0GwYF2zK28cLJlYujQf1\n"
    "Z6OCAYIwggF+MBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFIoj655r1/k3\n"
    "XfltITl2mqFn3hCoMB8GA1UdIwQYMBaAFLPbSKT5ocXYrjZBzBFjaWIpvEvGMA4G\n"
    "A1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwdgYI\n"
    "KwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5j\n"
    "b20wQAYIKwYBBQUHMAKGNGh0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdp\n"
    "Q2VydEdsb2JhbFJvb3RHMy5jcnQwQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2Ny\n"
    "bDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9vdEczLmNybDA9BgNVHSAE\n"
    "NjA0MAsGCWCGSAGG/WwCATAHBgVngQwBATAIBgZngQwBAgEwCAYGZ4EMAQICMAgG\n"
    "BmeBDAECAzAKBggqhkjOPQQDAwNoADBlAjB+Jlhu7ojsDN0VQe56uJmZcNFiZU+g\n"
    "IJ5HsVvBsmcxHcxyeq8ickBCbmWE/odLDxkCMQDmv9auNIdbP2fHHahv1RJ4teaH\n"
    "MUSpXca4eMzP79QyWBH/OoUGPB2Eb9P1+dozHKQ=\n"
    "-----END CERTIFICATE-----\n";
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PASSWORD
#define MAX_RETRY 5

static const char *TAG = "HTTP_CLIENT";
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static int retry_count = 0;

// Wi-Fiイベントハンドラ
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ESP_LOGI(TAG, "Connected to Wi-Fi and got IP address.");
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (retry_count < MAX_RETRY) {
      ESP_LOGI(TAG, "Retrying Wi-Fi connection... (%d/%d)", retry_count + 1,
               MAX_RETRY);
      esp_wifi_connect();
      retry_count++;
    } else {
      ESP_LOGE(TAG, "Wi-Fi connection failed. Rebooting...");
      esp_restart();
    }
  }
}

// Wi-Fi初期化
static void wifi_init(void) {
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_event_group = xEventGroupCreate();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &wifi_event_handler, NULL, NULL);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &wifi_event_handler, NULL, NULL);

  wifi_config_t wifi_config = {
      .sta = {.ssid = WIFI_SSID, .password = WIFI_PASSWORD},
  };
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  ESP_LOGI(TAG, "Waiting for IP ...");
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE,
                      portMAX_DELAY);
}

// HTTPレスポンスハンドラ
static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
             evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    if (evt->data_len > 0) {
      ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      printf("%.*s", evt->data_len, (char *)evt->data);
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}

void app_main(void) {
  // NVSの初期化
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Wi-Fi接続
  wifi_init();

  // HTTPSクライアント設定
  esp_http_client_config_t config = {
      .url = "https://example.com",
      .event_handler = _http_event_handler,
      .cert_pem = server_cert_pem, // ルート証明書を設定
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  // GETリクエストの送信
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "HTTP GET Status = %d",
             esp_http_client_get_status_code(client));
  } else {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
  }

  // クライアントのクリーンアップ
  esp_http_client_cleanup(client);
}
