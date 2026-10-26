#include "scr_splash.h"
#include "screens.h"
#include "ui.h"             /* ui_scrSplash, ui_lblAppName, ui_lblLoading (SquareLine) */
#include "bsp_display.h"
#include "esp_log.h"

static const char *TAG = "scr_splash";

#define SPLASH_FADE_IN_MS      800
#define SPLASH_DOT_STEP_MS     400                       /* tiempo de cada estado */
#define SPLASH_DOT_FRAMES      4                         /* "", ".", "..", "..." */

static const char *const s_loading_frames[SPLASH_DOT_FRAMES] = {
    "Iniciando",
    "Iniciando.",
    "Iniciando..",
    "Iniciando...",
};

static bool    s_backlight_faded = false;
static int32_t s_last_frame      = -1;

/* ------------------------------------------------------------------ */
/* Animación de puntos suspensivos (lv_anim en loop)                  */
/* ------------------------------------------------------------------ */

/* La animación recorre 0 -> SPLASH_DOT_FRAMES de forma lineal; la parte
 * entera del valor es el frame a mostrar. Sólo se toca el label cuando el
 * frame cambia, para no invalidar la pantalla en cada tick de animación. */
static void dots_anim_cb(void *var, int32_t value)
{
    int32_t frame = value;
    if (frame >= SPLASH_DOT_FRAMES) frame = SPLASH_DOT_FRAMES - 1;
    if (frame < 0)                  frame = 0;
    if (frame == s_last_frame) return;

    s_last_frame = frame;
    lv_label_set_text_static((lv_obj_t *)var, s_loading_frames[frame]);
}

static void dots_anim_start(void)
{
    s_last_frame = -1;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui_lblLoading);
    lv_anim_set_exec_cb(&a, dots_anim_cb);
    lv_anim_set_values(&a, 0, SPLASH_DOT_FRAMES);
    lv_anim_set_time(&a, SPLASH_DOT_STEP_MS * SPLASH_DOT_FRAMES);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
}

static void dots_anim_stop(void)
{
    lv_anim_del(ui_lblLoading, dots_anim_cb);
}

/* ------------------------------------------------------------------ */
/* Toque en cualquier punto de la pantalla                            */
/* ------------------------------------------------------------------ */

static void on_splash_clicked(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "touch -> home");
    ui_screens_navigate(SCREEN_HOME);
}

/* ------------------------------------------------------------------ */

void scr_splash_prepare(void)
{
    /* La pantalla entera es la zona táctil: no hay botón visible. */
    lv_obj_add_flag(ui_scrSplash, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ui_scrSplash, LV_OBJ_FLAG_SCROLLABLE);

    /* Si el toque cae sobre un hijo (p. ej. un label marcado como clickable
     * en SquareLine), el evento sube ("bubble") hasta la pantalla. */
    uint32_t child_cnt = lv_obj_get_child_cnt(ui_scrSplash);
    for (uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_add_flag(lv_obj_get_child(ui_scrSplash, i), LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    /* CLICKED (press + release) y no PRESSED: así la liberación del dedo
     * no termina actuando sobre la pantalla Home recién cargada. */
    lv_obj_add_event_cb(ui_scrSplash, on_splash_clicked, LV_EVENT_CLICKED, NULL);
    ESP_LOGD(TAG, "prepare");
}

void scr_splash_init(void)
{
    /* La pantalla Splash es la primera que carga ui_init(), así que su init()
     * corre en el primer tick de UI: es el momento del fade-in de arranque. */
    if (!s_backlight_faded) {
        bsp_display_fade_in(SPLASH_FADE_IN_MS);
        s_backlight_faded = true;
    }
    dots_anim_start();
    ESP_LOGD(TAG, "init");
}

void scr_splash_uninit(void)
{
    dots_anim_stop();
    ESP_LOGD(TAG, "uninit");
}

void scr_splash_step(void)
{
    /* Nada por tick: todo lo dirigen la animación y el evento de toque. */
}
