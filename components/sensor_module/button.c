#include "button.h"

static const char *TAG = "BUTTON";

void button_isr_handler(void *arg)
{
    button_t *btn = (button_t *)arg;
    int level = gpio_get_level(btn->pin);

    // Nếu trạng thái thay đổi -> start timer debounce
    if (level != btn->last_state) {
        btn->last_state = level;
        esp_timer_stop(btn->timer);
        esp_timer_start_once(btn->timer, btn->debounce_time_us);
    }
}

static void button_debounce_timer_cb(void *arg)
{
    button_t *btn = (button_t *)arg;
    int level = gpio_get_level(btn->pin);

    // Nếu ổn định và bằng active_level → gọi callback
    if (level == btn->active_level && level != btn->stable_state) {
        btn->stable_state = level;
        if (btn->callback)
            btn->callback(btn->callback_arg);
    } else if (level != btn->active_level) {
        btn->stable_state = level; // reset lại
    }
}

esp_err_t button_init(button_t *btn, gpio_num_t pin, int active_level,
                      uint64_t debounce_us, button_callback_t cb, void *arg)
{
    btn->pin = pin;
    btn->debounce_time_us = debounce_us;
    btn->callback = cb;
    btn->callback_arg = arg;
    btn->active_level = active_level;
    btn->last_state = gpio_get_level(pin);
    btn->stable_state = btn->last_state;

    // Config GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = (active_level == 0) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = (active_level == 1) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    // Tạo timer debounce
    const esp_timer_create_args_t timer_args = {
        .callback = button_debounce_timer_cb,
        .arg = btn,
        .name = "button_timer"
    };
    esp_timer_create(&timer_args, &btn->timer);

    // Đăng ký ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, button_isr_handler, btn);

    ESP_LOGI(TAG, "Button initialized on GPIO %d", pin);
    return ESP_OK;
}
