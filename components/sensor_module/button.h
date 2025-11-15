#ifndef BUTTON_H
#define BUTTON_H

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

typedef void (*button_callback_t)(void *arg);

typedef struct {
    gpio_num_t pin;
    uint64_t debounce_time_us;
    int last_state;
    int stable_state;
    int active_level;
    button_callback_t callback;
    void *callback_arg;
    esp_timer_handle_t timer;
} button_t;

esp_err_t button_init(button_t *btn, gpio_num_t pin, int active_level, uint64_t debounce_us, button_callback_t cb, void *arg);
void button_isr_handler(void *arg);

#endif