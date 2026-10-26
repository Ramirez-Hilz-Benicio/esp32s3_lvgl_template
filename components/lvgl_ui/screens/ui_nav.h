#ifndef __UI_NAV_H__
#define __UI_NAV_H__

#include "lvgl.h"
#include "screens.h"

/** Alto de la barra inferior: las pantallas de SquareLine deben dejar libre
 *  esta franja abajo (no poner widgets en los últimos ~40 px). */
#define UI_NAV_BAR_HEIGHT  36

/**
 * Crea UNA sola vez la barra de navegación inferior sobre lv_layer_top():
 * al no pertenecer a ninguna pantalla, se ve igual en todas y no hace falta
 * duplicarla en SquareLine. Llamar después de ui_init().
 */
void ui_nav_init(void);

/** Resalta el ícono de la pantalla activa (lo llama el motor de pantallas). */
void ui_nav_set_active(ui_screens_t screen);

/**
 * Registra en `scr` el handler de swipe horizontal:
 *   swipe a la izquierda -> pantalla siguiente
 *   swipe a la derecha   -> pantalla anterior
 */
void ui_nav_attach_gestures(lv_obj_t *scr);

/**
 * Evita que arrastrar `obj` (slider, switch…) se interprete como swipe de
 * pantalla: el gesto ya no "burbujea" hasta la pantalla.
 */
void ui_nav_block_gestures(lv_obj_t *obj);

#endif
