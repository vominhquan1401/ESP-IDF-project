#pragma once
#include "esp_err.h"

// Khởi chạy portal cấu hình (AP + webserver)
esp_err_t wifi_config_portal_start(void);

// Dừng portal (tắt webserver; AP sẽ do wifi_manager điều khiển)
void wifi_config_portal_stop(void);
