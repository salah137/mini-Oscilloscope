#ifndef WIFI_STA_H
#define WIFI_STA_H

#define WIFI_STA_CONNECTED BIT0
#define WIFI_IPV4_OBTAINED BIT1
#define WIFI_IPV6_OBTAINED BIT2

#include "esp_err.h"
#include "freertos/idf_additions.h"

esp_err_t wifi_sta_init(EventGroupHandle_t evnt_grp);

esp_err_t wifi_sta_stop(void);

esp_err_t wifi_sta_reconnect(void);

#endif