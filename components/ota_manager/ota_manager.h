#pragma once
#include "esp_err.h"

// Thực hiện OTA update (blocking)
esp_err_t ota_manager_run(const char *url);

// (Tùy chọn) thực hiện OTA trong task riêng
void ota_manager_task(void *arg);
