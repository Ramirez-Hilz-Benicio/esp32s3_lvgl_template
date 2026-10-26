#include "scr_settings.h"
#include "screens.h"
#include "screens_timeout.h"
#include "ui_nav.h"
#include "lvgl_ui.h"        /* UI_SCREEN_OFF_TIMEOUT_MS */
#include "ui.h"             /* ui_scrSettings, ui_sldBrightness, ui_lblBrightness, ui_swTimeout, ui_lblBuildDate */

#include "bsp_display.h"
#include "esp_log.h"

static const char *TAG = "scr_settings";

/* Valores por defecto (los restaura el botón físico) */
#define DEFAULT_BRIGHTNESS      100
#define DEFAULT_TIMEOUT_ON      true

/* Brillo mínimo del slider: con 0 % la pantalla quedaría negra y no se
 * vería el slider para volver a subirlo. */
#define BRIGHTNESS_MIN          5
#define BRIGHTNESS_MAX          100

/* ------------------------------------------------------------------ */
/* Brillo                                                             */
/* ------------------------------------------------------------------ */

static void apply_brightness(int32_t value)
{
    bsp_display_set_brightness((uint8_t)value);
    lv_label_set_text_fmt(ui_lblBrightness, "%d %%", (int)value);
}

/* VALUE_CHANGED llega en cada paso mientras se arrastra el slider, así que el
 * brillo cambia físicamente en tiempo real. */
static void on_brightness_changed(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    apply_brightness(lv_slider_get_value(slider));
}

/* ------------------------------------------------------------------ */
/* Timeout de pantalla                                                */
/* ------------------------------------------------------------------ */

static void apply_timeout(bool enabled)
{
    disp_timeout_set_off_ms(enabled ? UI_SCREEN_OFF_TIMEOUT_MS : TIMEOUT_NEVER);
    ESP_LOGI(TAG, "screen timeout %s", enabled ? "ON" : "OFF");
}

static void on_timeout_switch_changed(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    apply_timeout(lv_obj_has_state(sw, LV_STATE_CHECKED));
}

static void set_switch(lv_obj_t *sw, bool on)
{
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    else    lv_obj_clear_state(sw, LV_STATE_CHECKED);
}

/* Lleva los widgets al estado real del sistema (brillo actual y timeout). */
static void sync_widgets(void)
{
    int32_t brightness = bsp_display_get_brightness();
    if (brightness < BRIGHTNESS_MIN) brightness = BRIGHTNESS_MIN;
    lv_slider_set_value(ui_sldBrightness, brightness, LV_ANIM_OFF);
    lv_label_set_text_fmt(ui_lblBrightness, "%d %%", (int)brightness);
    set_switch(ui_swTimeout, disp_timeout_get_off_ms() != TIMEOUT_NEVER);
}

/* ------------------------------------------------------------------ */

void scr_settings_prepare(void)
{
    lv_slider_set_range(ui_sldBrightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    lv_obj_add_event_cb(ui_sldBrightness, on_brightness_changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_swTimeout, on_timeout_switch_changed, LV_EVENT_VALUE_CHANGED, NULL);

    /* Arrastrar el slider no debe cambiar de pantalla (el switch sí deja
     * pasar el swipe: sólo reacciona a toques). */
    ui_nav_block_gestures(ui_sldBrightness);
    ui_nav_attach_gestures(ui_scrSettings);

    lv_label_set_text(ui_lblBuildDate, "Compilado: " __DATE__ " " __TIME__);
    sync_widgets();
    ESP_LOGD(TAG, "prepare");
}

void scr_settings_init(void)
{
    sync_widgets();
    ESP_LOGD(TAG, "init");
}

void scr_settings_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
}

void scr_settings_step(void)
{
}

void scr_settings_on_button(void)
{
    ESP_LOGI(TAG, "button -> restore defaults");
    lv_slider_set_value(ui_sldBrightness, DEFAULT_BRIGHTNESS, LV_ANIM_ON);
    apply_brightness(DEFAULT_BRIGHTNESS);
    set_switch(ui_swTimeout, DEFAULT_TIMEOUT_ON);
    apply_timeout(DEFAULT_TIMEOUT_ON);
}
