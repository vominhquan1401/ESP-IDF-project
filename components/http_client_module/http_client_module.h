#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t send_sensor_data_to_server(void);
int get_send_interval_ms(void);

#ifdef __cplusplus
}
#endif
