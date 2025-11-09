#pragma once
#include "esp_err.h"
#include "esp_wifi_types.h"
#ifdef __cplusplus
extern "C"
{
#endif
    // Khởi chạy portal cấu hình (AP + webserver)
    extern bool portal_done;
    esp_err_t wifi_config_portal_start(void);

    // Dừng portal (tắt webserver; AP sẽ do wifi_manager điều khiển)
    void wifi_config_portal_stop(void);
#ifdef __cplusplus
}
#endif
