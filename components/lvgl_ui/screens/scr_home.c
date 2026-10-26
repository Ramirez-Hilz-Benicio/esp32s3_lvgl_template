#include "scr_home.h"
#include "screens.h"
#include "ui_nav.h"
#include "ui.h"             /* ui_scrHome, ui_arcBattery, ui_lblBattery, ui_lblTemp, ui_lblUptime, ui_lblStatus */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bsp_battery.h"
#include "bsp_temp.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "scr_home";

#define HOME_REFRESH_MS     1000

/* Batería LiPo de 1 celda: 3.30 V se toma como vacía y 4.20 V como llena. */
#define BATT_EMPTY_V        3.30f
#define BATT_FULL_V         4.20f

/* Filtrado de la lectura del ADC (es ruidosa): se promedian varias muestras
 * por refresco, se suaviza con un filtro exponencial (EMA) entre refrescos y
 * el porcentaje mostrado sólo cambia si se mueve al menos BATT_HYST_PCT. */
#define BATT_SAMPLES        16
#define BATT_EMA_ALPHA      0.2f
#define BATT_HYST_PCT       2

/* Umbrales de color del arco */
#define BATT_LOW_PCT        20
#define BATT_MID_PCT        50

/* Umbrales del indicador de estado (temperatura del chip, °C) */
#define TEMP_ALERT_C        45.0f
#define TEMP_CRIT_C         60.0f

typedef enum {
    TEMP_STATE_UNKNOWN = 0,
    TEMP_STATE_NORMAL,
    TEMP_STATE_ALERT,
    TEMP_STATE_CRITICAL,
} temp_state_t;

static lv_timer_t *s_refresh_timer = NULL;
static float       s_batt_filtered_v = -1.0f;   /* < 0: todavía sin lecturas */
static int         s_batt_shown_pct  = -1;

/* ------------------------------------------------------------------ */
/* Batería — arco + porcentaje                                        */
/* ------------------------------------------------------------------ */

static float battery_read_voltage_avg(void)
{
    float sum = 0.0f;
    for (int i = 0; i < BATT_SAMPLES; i++) {
        float v = 0.0f;   /* queda en 0 si el ADC no está calibrado */
        bsp_battery_get_voltage(&v, NULL);
        sum += v;
    }
    return sum / BATT_SAMPLES;
}

static int battery_read_percent(void)
{
    float voltage = battery_read_voltage_avg();

    /* Filtro exponencial: cada lectura nueva aporta sólo un 20 %. */
    if (s_batt_filtered_v < 0.0f) {
        s_batt_filtered_v = voltage;
    } else {
        s_batt_filtered_v += BATT_EMA_ALPHA * (voltage - s_batt_filtered_v);
    }

    float pct = (s_batt_filtered_v - BATT_EMPTY_V) * 100.0f / (BATT_FULL_V - BATT_EMPTY_V);
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    int pct_i = (int)(pct + 0.5f);

    /* Histéresis: ignora variaciones menores a BATT_HYST_PCT. */
    if (s_batt_shown_pct < 0 || abs(pct_i - s_batt_shown_pct) >= BATT_HYST_PCT) {
        s_batt_shown_pct = pct_i;
    }

    ESP_LOGD(TAG, "battery %.3f V (filtered %.3f V) -> %d %%",
             voltage, s_batt_filtered_v, s_batt_shown_pct);
    return s_batt_shown_pct;
}

static void battery_apply_color(int pct)
{
    lv_palette_t palette;
    if (pct <= BATT_LOW_PCT)      palette = LV_PALETTE_RED;
    else if (pct <= BATT_MID_PCT) palette = LV_PALETTE_AMBER;
    else                          palette = LV_PALETTE_GREEN;

    lv_obj_set_style_arc_color(ui_arcBattery, lv_palette_main(palette),
                               LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

static void update_battery(void)
{
    int pct = battery_read_percent();
    lv_arc_set_value(ui_arcBattery, (int16_t)pct);
    lv_label_set_text_fmt(ui_lblBattery, "%d %%", pct);
    battery_apply_color(pct);
}

/* ------------------------------------------------------------------ */
/* Temperatura del chip                                               */
/* ------------------------------------------------------------------ */

/* Indicador de estado general: ícono + texto + color según la temperatura. */
static void update_status(temp_state_t state)
{
    const char *text;
    lv_color_t  color;

    switch (state) {
        case TEMP_STATE_NORMAL:
            text  = LV_SYMBOL_OK " Normal";
            color = lv_palette_main(LV_PALETTE_GREEN);
            break;
        case TEMP_STATE_ALERT:
            text  = LV_SYMBOL_WARNING " Alerta";
            color = lv_palette_main(LV_PALETTE_AMBER);
            break;
        case TEMP_STATE_CRITICAL:
            text  = LV_SYMBOL_CLOSE " Critico";
            color = lv_palette_main(LV_PALETTE_RED);
            break;
        default:
            text  = LV_SYMBOL_REFRESH " Sin datos";
            color = lv_palette_main(LV_PALETTE_GREY);
            break;
    }

    lv_label_set_text(ui_lblStatus, text);
    lv_obj_set_style_text_color(ui_lblStatus, color, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void update_temperature(void)
{
    float        celsius = 0.0f;
    char         buf[32];
    temp_state_t state   = TEMP_STATE_UNKNOWN;

    if (bsp_temp_sensor_read(&celsius) == ESP_OK) {
        /* snprintf de la libc: el printf interno de LVGL no formatea floats
         * (CONFIG_LV_SPRINTF_USE_FLOAT está deshabilitado). */
        snprintf(buf, sizeof(buf), "Temp: %.1f \xC2\xB0" "C", celsius);   /* "°" en UTF-8 */
        if (celsius >= TEMP_CRIT_C)       state = TEMP_STATE_CRITICAL;
        else if (celsius >= TEMP_ALERT_C) state = TEMP_STATE_ALERT;
        else                              state = TEMP_STATE_NORMAL;
    } else {
        snprintf(buf, sizeof(buf), "Temp: -- \xC2\xB0" "C");
    }
    lv_label_set_text(ui_lblTemp, buf);
    update_status(state);
}

/* ------------------------------------------------------------------ */
/* Uptime HH:MM:SS                                                    */
/* ------------------------------------------------------------------ */

static void update_uptime(void)
{
    uint32_t total_s = (uint32_t)(esp_timer_get_time() / 1000000LL);
    unsigned hours   = (unsigned)(total_s / 3600U);
    unsigned minutes = (unsigned)((total_s / 60U) % 60U);
    unsigned seconds = (unsigned)(total_s % 60U);

    lv_label_set_text_fmt(ui_lblUptime, "%02u:%02u:%02u", hours, minutes, seconds);
}

/* ------------------------------------------------------------------ */

static void refresh_all(void)
{
    update_battery();
    update_temperature();
    update_uptime();
}

static void refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    refresh_all();
}

/* LV_EVENT_SCREEN_LOAD_START llega justo cuando la pantalla empieza a
 * cargarse: refrescamos antes del primer dibujado para que nunca se vea
 * el texto de ejemplo que quedó en SquareLine. */
static void on_home_load_start(lv_event_t *e)
{
    (void)e;
    refresh_all();
}

/* ------------------------------------------------------------------ */

void scr_home_prepare(void)
{
    /* Arco de batería tipo "medidor": 270° de fondo, de 135° a 45°
     * (abierto por abajo), rango 0-100 y sin interacción táctil. */
    lv_arc_set_mode(ui_arcBattery, LV_ARC_MODE_NORMAL);
    lv_arc_set_range(ui_arcBattery, 0, 100);
    lv_arc_set_bg_angles(ui_arcBattery, 135, 45);
    lv_arc_set_rotation(ui_arcBattery, 0);
    lv_obj_clear_flag(ui_arcBattery, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_style(ui_arcBattery, NULL, LV_PART_KNOB);   /* sin perilla: es sólo indicador */

    lv_obj_add_event_cb(ui_scrHome, on_home_load_start, LV_EVENT_SCREEN_LOAD_START, NULL);
    ui_nav_attach_gestures(ui_scrHome);
    ESP_LOGD(TAG, "prepare");
}

void scr_home_init(void)
{
    /* Timer LVGL de 1 s: corre en el contexto de LVGL, por eso puede
     * llamar lv_* directamente sin tareas FreeRTOS ni cola de updates. */
    if (s_refresh_timer == NULL) {
        s_refresh_timer = lv_timer_create(refresh_timer_cb, HOME_REFRESH_MS, NULL);
    }
    refresh_all();
    ESP_LOGD(TAG, "init");
}

void scr_home_uninit(void)
{
    if (s_refresh_timer) {
        lv_timer_del(s_refresh_timer);
        s_refresh_timer = NULL;
    }
    ESP_LOGD(TAG, "uninit");
}

void scr_home_on_button(void)
{
    /* Botón físico en Home: refresco inmediato de los tres valores, sin
     * esperar al próximo tick del timer de 1 s. */
    ESP_LOGI(TAG, "button -> force refresh");
    s_batt_shown_pct = -1;   /* que la histéresis no oculte el valor nuevo */
    refresh_all();
    if (s_refresh_timer) {
        lv_timer_reset(s_refresh_timer);
    }
}

void scr_home_step(void)
{
    /* El refresco lo maneja el lv_timer de 1 s. */
}
