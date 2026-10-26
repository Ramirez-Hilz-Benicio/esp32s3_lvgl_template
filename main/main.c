#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_touch.h"
#include "bsp_lvgl.h"
#include "bsp_battery.h"
#include "bsp_temp.h"

#include "lvgl_ui.h"

/** @brief App entry point */
void app_main(void) {
    // Initialize necessary peripherals to handle display and touch
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_init();
    bsp_display_init();
    bsp_touch_init(i2c_bus);
    ESP_ERROR_CHECK(bsp_lvgl_init());
    // Backlight starts off: the splash screen fades it in (bsp_display_fade_in)
    bsp_display_brightness_init();
    bsp_display_set_brightness(0);
    // Peripherals read by the home screen
    bsp_battery_init();
    if (bsp_temp_sensor_init() != ESP_OK) {
        ESP_LOGW("main", "temperature sensor not available");
    }
    // Always take LVGL mutex before doing anything with LVGL
    if (lvgl_port_lock(0)) {
        lvgl_ui_start();
        lvgl_port_unlock();
    }
}