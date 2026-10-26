#ifndef __SCR_SETTINGS_H__
#define __SCR_SETTINGS_H__

/**
 * Settings screen controller (SquareLine: scrSettings).
 *
 *  - sldBrightness / lblBrightness : brillo de la retroiluminación (LEDC) en tiempo real
 *  - swTimeout                     : habilita/deshabilita el apagado por inactividad
 *  - lblBuildDate                  : fecha y hora de compilación (__DATE__, __TIME__)
 */
void scr_settings_prepare(void);
void scr_settings_init(void);
void scr_settings_uninit(void);
void scr_settings_step(void);

/** Acción del botón físico en esta pantalla: restaura brillo y switch por defecto. */
void scr_settings_on_button(void);

#endif
