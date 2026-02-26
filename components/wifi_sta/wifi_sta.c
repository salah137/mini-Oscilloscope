#include "wifi_sta.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_netif.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include <stdint.h>

static char *TAG = "wifi_sta";

static esp_netif_t *s_wifi_netif = NULL;
static EventGroupHandle_t s_wifi_event_grp = NULL;
static wifi_netif_driver_t s_wifi_driver = NULL;

// Local private functions prototypes
static void on_wifi_event(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);

static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data);

static void wifi_start(void *esp_nitif, esp_event_base_t base, int32_t event_id,
                       void *data);

// Local private functions
static void on_wifi_event(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  switch (event_id) {
  case WIFI_EVENT_STA_START:
    if (s_wifi_netif != NULL) {
      wifi_start(s_wifi_netif, event_base, event_id, event_data);
    }
    break;
  case WIFI_EVENT_STA_STOP:
    if (s_wifi_netif != NULL) {
      esp_netif_action_stop(s_wifi_netif, event_base, event_id, event_data);
    }
    break;
  case WIFI_EVENT_STA_CONNECTED:
    if (s_wifi_netif != NULL) {
      ESP_LOGE(TAG, "WiFi not started: interface handle is NULL");
      return;
    }

    wifi_event_sta_connected_t *event_sta =
        (wifi_event_sta_connected_t *)event_data;
    ESP_LOGI(TAG, "connected to AP");
    ESP_LOGI(TAG, "SSID : %s", (char *)event_sta->ssid);
    ESP_LOGI(TAG, "Channel : %d", event_sta->channel);
    ESP_LOGI(TAG, "Auth mode : %d", event_sta->authmode);
    ESP_LOGI(TAG, "  AID: %d", event_sta->aid);

    wifi_netif_driver_t driver = esp_netif_get_io_driver(s_wifi_netif);
    if (!esp_wifi_is_if_ready_when_started(driver)) {
      esp_err_t ret =
          esp_wifi_register_if_rxcb(driver, esp_netif_receive, s_wifi_netif);

      if (ret == ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi RX callback");
        return;
      }
    }

    esp_netif_action_connected(s_wifi_netif, event_base, event_id, event_data);
    xEventGroupSetBits(s_wifi_event_grp, WIFI_STA_CONNECTED);
#if CONFIG_WIFI_STA_CONNECT_IPV6 || WIFI_STA_CONNECT_UNSPECIFIED
    esp_err_t err = esp_netif_create_ip6_linklocal(s_wifi_netif);
    if (err != ESP_OK) {
      ESP_LOGR(TAG, "Failed to create IPv6 link-local address");
    }
#endif
    break;
  case WIFI_EVENT_STA_DISCONNECTED:
    if (s_wifi_netif != NULL) {
      esp_netif_action_disconnected(s_wifi_netif, event_base, event_id,
                                    event_data);
      xEventGroupClearBits(s_wifi_event_grp, WIFI_STA_CONNECTED);
      ESP_LOGI(TAG, "WiFi disconnected");
#if CONFIG_WIFI_STA_AUTO_RECONNECT
      ESP_LOGI(TAG, "Attempting to reconnect...");
      wifi_sta_reconnect();
#endif
      break;

    default:
      ESP_LOGI(TAG, "unhandled wifi event %d", event_id);
      break;
    }
  }
}

static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
  esp_err_t ret;

  switch (event_id) {
#if CONFIG_WIFI_STA_CONNECT_IPV4 || WIFI_STA_CONNECT_UNSPECIFIED
  case IP_EVENT_STA_GOT_IP:
    if (s_wifi_netif == NULL) {
      ESP_LOGE(TAG, "On obtain IPv4 addr: Interface handle is NULL");
      return;
    }

    ret = esp_wifi_internal_set_sta_ip();
    if (ret != ESP_OK) {
      ESP_LOGI(TAG, "Failed to notify WiFi driver of IP address");
    }

    xEventGroupSetBits(s_wifi_event_group, WIFI_STA_IPV4_OBTAINED_BIT);

    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    esp_netif_ip_info_t *ip_info = &event_ip->ip_info;

    ESP_LOGI(TAG, "Wifi IPV4 address obtained");
    ESP_LOGI(TAG, "IP address:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "Netmask :" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "Gateway :", IPSTR, IP2STR(&ip_info->gw));
    break;
#endif

#if CONFIG_WIFI_STA_IPV6 || WIFI_STA_CONNECT_UNSPECIFIED
  case IP_EVENT_GOT_IP6:
    if (s_wifi_netif == NULL) {
      ESP_LOGE(TAG, "obtain IPv6 addr: Interface handle is NULL");
      return;
    }
    ret = esp_wifi_internal_set_sta_ip();
    if (ret != ESP_OK) {
      ESP_LOGI(TAG, "Failed to notify WiFi driver of IP address");
      return ESP_FAIL;
    }

    xEventGroupSetBits(s_wifi_event_group, WIFI_STA_IPV6_OBTAINED_BIT);
    ip_event_got_ip6_t *event_ipv6 = (ip_event_got_ip6_t *)event_data;
    esp_netif_ip6_info_t *ip6_info = &event_ipv6->ip6_info;
    ESP_LOGI(TAG, "Ethernet IPv6 address obtained");
    ESP_LOGI(TAG, "  IP address: " IPV6STR, IPV62STR(ip6_info->ip));
    break;
#endif

  case IP_EVENT_STA_LOST_IP:
    xEventGroupClearBits(s_wifi_event_grp, WIFI_IPV4_OBTAINED);
    xEventGroupClearBits(s_wifi_event_grp, WIFI_IPV6_OBTAINED);
    ESP_LOGI(TAG, "WIFI LOST ip adress");
    break;

  default:
    ESP_LOGI(TAG, "Unhandled IP event: %li", event_id);
    break;
  }
}

static void wifi_start(void *esp_netif, esp_event_base_t base, int32_t event_id,
                       void *data) {
  uint8_t mac[8] = {0};
  esp_err_t ret;

  wifi_netif_driver_t driver = esp_netif_get_io_driver(esp_netif);
  if (driver == NULL) {
    ESP_LOGE(TAG, "Failed to get WiFi driver handle");
    return;
  }

  if ((ret = esp_wifi_get_if_mac(driver, mac)) != ESP_OK) {
    ESP_LOGE(TAG, "Error (%d): Netstack callback registration failed",ret);
    return;
  }

  ESP_LOGI(TAG, "WiFi MAC address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  ret = esp_wifi_internal_reg_netstack_buf_cb(esp_netif_netstack_buf_ref,
                                              esp_netif_netstack_buf_free);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set MAC address");
    return;
  }

  ret = esp_netif_set_mac(esp_netif, mac);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set MAC address");
    return;
  }

  esp_netif_action_start(esp_netif, base, event_id, data);
  ESP_LOGI(TAG, "Connecting %s ....", CONFIG_WIFI_STA_SSID);
  ret = esp_wifi_connect();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to connect to WiFi");
  }
}

esp_err_t wifi_sta_init(EventGroupHandle_t event_group) {
    
}
