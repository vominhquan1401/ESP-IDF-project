#pragma once

#include "driver/i2c.h"
#include "esp_err.h"

// --- Cấu hình chân I2C ---
#define I2C_MASTER_SCL_IO           12
#define I2C_MASTER_SDA_IO           11
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define DHT20_ADDR                  0x38


// --- Khai báo hàm ---
esp_err_t i2c_master_init(void);
void dht20_init(void);
esp_err_t dht20_read(float *temperature, float *humidity);
