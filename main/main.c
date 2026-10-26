#include "esp_lvgl_port.h"
#include "esp_log.h"

#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_touch.h"
#include "bsp_lvgl.h"
#include "bsp_battery.h"
#include "bsp_temp.h"
#include "bsp_button.h"

#include "lvgl_ui.h"
#include "screens/screens_update.h"

/**
 * Callback del botón físico. Corre en la tarea de esp_timer del componente
 * espressif/button, NO en el contexto de LVGL: por eso no toca ningún objeto
 * lv_* y sólo encola el comando. El timer LVGL lo ejecuta de forma segura
 * en ui_update_screens() y lo despacha según la pantalla activa.
 */
static void on_physical_button_click(void)
{
    ui_update_cmd(UI_UPDATE_BUTTON_PRESS, NULL);
}

/** @brief App entry point */
void app_main(void) {
    // Initialize necessary peripherals to handle display and touch
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_init();
    bsp_display_init();
    bsp_touch_init(i2c_bus);
    ESP_ERROR_CHECK(bsp_lvgl_init());
    bsp_display_brightness_init();
    bsp_display_set_brightness(100);
    // Peripherals read by the Home screen
    bsp_battery_init();
    if (bsp_temp_sensor_init() != ESP_OK) {
        ESP_LOGW("main", "temperature sensor not available");
    }
    // Always take LVGL mutex before doing anything with LVGL
    if (lvgl_port_lock(0)) {
        lvgl_ui_start();
        lvgl_port_unlock();
    }
    // Physical button: registered after lvgl_ui_start() so the UI update
    // queue already exists when the first click is enqueued.
    if (bsp_button_init(on_physical_button_click) != ESP_OK) {
        ESP_LOGW("main", "physical button not available");
    }
}
