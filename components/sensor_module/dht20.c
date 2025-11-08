#include "dht20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void dht20_init(void)
{
    uint8_t cmd[3] = {0xBE, 0x08, 0x00};
    i2c_master_write_to_device(I2C_MASTER_NUM, DHT20_ADDR, cmd, 3, 1000 / portTICK_PERIOD_MS);
    vTaskDelay(pdMS_TO_TICKS(10));
}

esp_err_t dht20_read(float *temperature, float *humidity)
{
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t data[7];

    // Gửi lệnh đo
    i2c_master_write_to_device(I2C_MASTER_NUM, DHT20_ADDR, cmd, 3, 1000 / portTICK_PERIOD_MS);
    vTaskDelay(pdMS_TO_TICKS(80));

    // Đọc dữ liệu
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, DHT20_ADDR, data, 7, 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) return ret;

    // Kiểm tra bit busy
    if (data[0] & 0x80) return ESP_FAIL;

    uint32_t raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((data[3] & 0xF0) >> 4);
    uint32_t raw_temperature = (((uint32_t)(data[3] & 0x0F)) << 16) | ((uint32_t)data[4] << 8) | data[5];

    *humidity = ((float)raw_humidity / 1048576.0f) * 100.0f;
    *temperature = ((float)raw_temperature / 1048576.0f) * 200.0f - 50.0f;

    return ESP_OK;
}
