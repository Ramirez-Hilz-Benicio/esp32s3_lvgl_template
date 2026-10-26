#include "ui_nav.h"
#include "esp_log.h"

static const char *TAG = "ui_nav";

/* Un ítem por pantalla, en el mismo orden que ui_screens_t. */
static const char *const s_nav_icons[SCREEN_MAX] = {
    [SCREEN_HOME]     = LV_SYMBOL_HOME,
    [SCREEN_SYSTEM]   = LV_SYMBOL_LIST,
    [SCREEN_SETTINGS] = LV_SYMBOL_SETTINGS,
};
static const char *const s_nav_names[SCREEN_MAX] = {
    [SCREEN_HOME]     = "Home",
    [SCREEN_SYSTEM]   = "Sistema",
    [SCREEN_SETTINGS] = "Ajustes",
};

#define NAV_COLOR_BG        lv_color_hex(0x15171A)
#define NAV_COLOR_IDLE      lv_color_hex(0x7A7F87)
#define NAV_COLOR_ACTIVE    lv_palette_main(LV_PALETTE_BLUE)

static lv_obj_t *s_bar = NULL;
static lv_obj_t *s_items[SCREEN_MAX];
static lv_obj_t *s_labels[SCREEN_MAX];

/* ------------------------------------------------------------------ */
/* Barra inferior                                                     */
/* ------------------------------------------------------------------ */

static void on_nav_item_clicked(lv_event_t *e)
{
    ui_screens_t target  = (ui_screens_t)(uintptr_t)lv_event_get_user_data(e);
    ui_screens_t current = ui_screens_get_current();
    if (target == current || current >= SCREEN_MAX) return;

    /* La animación sigue la dirección del ícono tocado. */
    lv_scr_load_anim_t anim = (target > current) ? LV_SCR_LOAD_ANIM_MOVE_LEFT
                                                 : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    ui_screens_navigate_anim(target, anim);
}

void ui_nav_init(void)
{
    if (s_bar) return;   /* se crea una única vez */

    lv_coord_t width = lv_disp_get_hor_res(NULL);

    s_bar = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_bar);
    lv_obj_set_size(s_bar, width, UI_NAV_BAR_HEIGHT);
    lv_obj_align(s_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_bar, NAV_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(s_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(s_bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(s_bar, 1, 0);
    lv_obj_set_style_border_color(s_bar, lv_color_hex(0x30343A), 0);
    lv_obj_clear_flag(s_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_bar, LV_FLEX_FLOW_ROW);

    for (int i = 0; i < SCREEN_MAX; i++) {
        lv_obj_t *item = lv_obj_create(s_bar);
        lv_obj_remove_style_all(item);
        lv_obj_set_height(item, LV_PCT(100));
        lv_obj_set_flex_grow(item, 1);            /* tres columnas iguales */
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(item, on_nav_item_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text_fmt(label, "%s %s", s_nav_icons[i], s_nav_names[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label, NAV_COLOR_IDLE, 0);
        lv_obj_center(label);

        /* Indicador de sección activa: línea superior del ítem */
        lv_obj_set_style_border_side(item, LV_BORDER_SIDE_TOP, 0);
        lv_obj_set_style_border_width(item, 3, 0);
        lv_obj_set_style_border_color(item, NAV_COLOR_ACTIVE, 0);
        lv_obj_set_style_border_opa(item, LV_OPA_TRANSP, 0);

        s_items[i]  = item;
        s_labels[i] = label;
    }

    ui_nav_set_active(ui_screens_get_current());
    ESP_LOGD(TAG, "nav bar ready");
}

void ui_nav_set_active(ui_screens_t screen)
{
    if (!s_bar) return;
    for (int i = 0; i < SCREEN_MAX; i++) {
        bool active = (i == (int)screen);
        lv_obj_set_style_text_color(s_labels[i], active ? NAV_COLOR_ACTIVE : NAV_COLOR_IDLE, 0);
        lv_obj_set_style_border_opa(s_items[i], active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }
}

/* ------------------------------------------------------------------ */
/* Gestos de swipe                                                    */
/* ------------------------------------------------------------------ */

static void on_screen_gesture(lv_event_t *e)
{
    (void)e;
    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL) return;

    /* Guardas: ignorar el gesto si hay un scroll activo o si ya se está
     * animando un cambio de pantalla (swipes muy rápidos seguidos). */
    if (lv_indev_get_scroll_obj(indev) != NULL) return;
    if (ui_screens_is_transitioning()) return;

    ui_screens_t current = ui_screens_get_current();
    if (current >= SCREEN_MAX) return;

    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_LEFT && current + 1 < SCREEN_MAX) {
        ui_screens_navigate_anim((ui_screens_t)(current + 1), LV_SCR_LOAD_ANIM_MOVE_LEFT);
    } else if (dir == LV_DIR_RIGHT && current > 0) {
        ui_screens_navigate_anim((ui_screens_t)(current - 1), LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    } else {
        return;
    }

    /* Que el mismo toque no termine además en un CLICK sobre la pantalla nueva. */
    lv_indev_wait_release(indev);
}

void ui_nav_attach_gestures(lv_obj_t *scr)
{
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);   /* un scroll anularía el gesto */
    lv_obj_add_event_cb(scr, on_screen_gesture, LV_EVENT_GESTURE, NULL);
}

void ui_nav_block_gestures(lv_obj_t *obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
}
