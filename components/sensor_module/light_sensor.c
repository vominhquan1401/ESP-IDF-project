#include "light_sensor.h"
#include "driver/adc.h"

#define LIGHT_SENSOR_CHANNEL ADC1_CHANNEL_5  // GPIO34
#define ADC_MAX_VALUE        4095
#define ADC_REF_VOLTAGE      3.3f  // V (điện áp tham chiếu 3.3V)

esp_err_t light_sensor_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(LIGHT_SENSOR_CHANNEL, ADC_ATTEN_DB_11);
    return ESP_OK;
}

esp_err_t light_sensor_read(float *lux)
{
    int adc_raw = adc1_get_raw(LIGHT_SENSOR_CHANNEL); // Đọc giá trị ADC 0–4095
    if (adc_raw < 0) return ESP_FAIL;

    // Quy đổi sang điện áp (V)
    float voltage = ((float)adc_raw / ADC_MAX_VALUE) * ADC_REF_VOLTAGE;

    *lux = (voltage / ADC_REF_VOLTAGE) * 1000.0f;

    return ESP_OK;
}
