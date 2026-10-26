#include "scr_system.h"
#include "screens.h"
#include "ui_nav.h"
#include "ui.h"             /* ui_scrSystem, ui_lblCpuFreq, ui_lblHeapFree, ... (SquareLine) */

#include <stdint.h>

#include "esp_system.h"     /* esp_get_free_heap_size() */
#include "esp_idf_version.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_rom_sys.h"    /* esp_rom_get_cpu_ticks_per_us() */
#include "esp_log.h"

static const char *TAG = "scr_system";

#define SYSTEM_REFRESH_MS   1000

static lv_timer_t *s_heap_timer = NULL;
static uint32_t    s_heap_min   = UINT32_MAX;   /* mínimo propio, reseteable */

/* ------------------------------------------------------------------ */
/* Heap libre + mínimo histórico                                      */
/* ------------------------------------------------------------------ */

/* esp_get_minimum_free_heap_size() no se puede resetear, así que el mínimo
 * "histórico" lo lleva esta pantalla: un timer LVGL lo actualiza siempre
 * (también con la pantalla oculta) y el botón físico lo reinicia. */
static void heap_sample(void)
{
    uint32_t free_bytes = esp_get_free_heap_size();
    if (free_bytes < s_heap_min) {
        s_heap_min = free_bytes;
    }
    lv_label_set_text_fmt(ui_lblHeapFree, "Heap libre: %u KB", (unsigned)(free_bytes / 1024U));
    lv_label_set_text_fmt(ui_lblHeapMin,  "Heap minimo: %u KB", (unsigned)(s_heap_min / 1024U));
}

static void heap_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    heap_sample();
}

/* ------------------------------------------------------------------ */
/* Datos fijos del SoC (se leen una sola vez)                         */
/* ------------------------------------------------------------------ */

static const char *chip_model_name(esp_chip_model_t model)
{
    switch (model) {
        case CHIP_ESP32:   return "ESP32";
        case CHIP_ESP32S2: return "ESP32-S2";
        case CHIP_ESP32S3: return "ESP32-S3";
        case CHIP_ESP32C3: return "ESP32-C3";
        default:           return "ESP32-?";
    }
}

static void fill_static_info(void)
{
    lv_label_set_text_fmt(ui_lblCpuFreq, "CPU: %u MHz", (unsigned)esp_rom_get_cpu_ticks_per_us());

    lv_label_set_text_fmt(ui_lblIdfVersion, "ESP-IDF: %s", esp_get_idf_version());

    esp_chip_info_t info;
    esp_chip_info(&info);
    lv_label_set_text_fmt(ui_lblChipModel, "Chip: %s rev v%u.%u, %u nucleos",
                          chip_model_name(info.model),
                          (unsigned)(info.revision / 100U), (unsigned)(info.revision % 100U),
                          (unsigned)info.cores);

    uint32_t flash_bytes = 0;
    if (esp_flash_get_size(NULL, &flash_bytes) == ESP_OK) {
        lv_label_set_text_fmt(ui_lblFlashSize, "Flash: %u MB", (unsigned)(flash_bytes / (1024U * 1024U)));
    } else {
        lv_label_set_text(ui_lblFlashSize, "Flash: --");
    }
}

/* ------------------------------------------------------------------ */

void scr_system_prepare(void)
{
    fill_static_info();
    heap_sample();
    s_heap_timer = lv_timer_create(heap_timer_cb, SYSTEM_REFRESH_MS, NULL);
    ui_nav_attach_gestures(ui_scrSystem);
    ESP_LOGD(TAG, "prepare");
}

void scr_system_init(void)
{
    heap_sample();   /* valores frescos al entrar, sin esperar 1 s */
    ESP_LOGD(TAG, "init");
}

void scr_system_uninit(void)
{
    ESP_LOGD(TAG, "uninit");
}

void scr_system_step(void)
{
}

void scr_system_on_button(void)
{
    ESP_LOGI(TAG, "button -> reset heap minimum");
    s_heap_min = UINT32_MAX;
    heap_sample();   /* el mínimo arranca de nuevo desde el valor actual */
    if (s_heap_timer) {
        lv_timer_reset(s_heap_timer);
    }
}
