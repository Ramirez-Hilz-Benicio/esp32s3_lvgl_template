#include "lvgl_ui.h"
#include "ui.h"
#include "screens.h"
#include "screens_update.h"
#include "screens_timeout.h"
#include "ui_nav.h"

#define UI_STEP_PERIOD_MS 20

static void ui_step_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ui_screens_step();
    ui_update_screens();
    disp_timeout_step();
}

void lvgl_ui_start(void)
{
    ui_update_init();
    /* Apagado de pantalla tras 30 s sin tocarla (se activa/desactiva desde
     * el switch de la pantalla Ajustes). */
    disp_timeout_init(TIMEOUT_NEVER, UI_SCREEN_OFF_TIMEOUT_MS);

    ui_init();              /* Squareline: create all LVGL screen objects */
    ui_nav_init();          /* barra de navegación inferior persistente (lv_layer_top) */
    ui_screens_prepare();   /* call prepare() on every registered screen */

    lv_timer_create(ui_step_timer_cb, UI_STEP_PERIOD_MS, NULL);
}
