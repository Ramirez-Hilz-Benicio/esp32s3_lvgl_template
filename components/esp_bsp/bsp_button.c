#include "bsp_button.h"

#include "iot_button.h"
#include "button_gpio.h"
#include "esp_log.h"

static const char *TAG = "bsp_button";

static button_handle_t       s_button   = NULL;
static bsp_button_click_cb_t s_on_click = NULL;

static void on_single_click(void *button_handle, void *usr_data)
{
    (void)button_handle;
    (void)usr_data;
    ESP_LOGI(TAG, "click");
    if (s_on_click) {
        s_on_click();
    }
}

esp_err_t bsp_button_init(bsp_button_click_cb_t on_click)
{
    s_on_click = on_click;

    const button_config_t btn_cfg = {
        .long_press_time  = 0,   /* 0 = valores por defecto del componente */
        .short_press_time = 0,
    };
    const button_gpio_config_t gpio_cfg = {
        .gpio_num          = BSP_PIN_BUTTON,
        .active_level      = BSP_BUTTON_ACTIVE_LEVEL,
        .enable_power_save = false,
        .disable_pull      = false,
    };

    esp_err_t err = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &s_button);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "create failed: %s", esp_err_to_name(err));
        return err;
    }

    err = iot_button_register_cb(s_button, BUTTON_SINGLE_CLICK, NULL, on_single_click, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "register cb failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "button ready on GPIO %d", (int)BSP_PIN_BUTTON);
    return ESP_OK;
}
