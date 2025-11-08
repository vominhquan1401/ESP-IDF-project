#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void update_simulated_sensors(void);
    float get_temperature(void);
    float get_humidity(void);
    char *get_current_timestamp(void);
    void initialize_sntp(void);
    void sensor_data_sender_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
