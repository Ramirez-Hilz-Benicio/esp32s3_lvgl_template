#ifndef __SCR_HOME_H__
#define __SCR_HOME_H__

/**
 * Home screen controller (SquareLine: scrHome).
 *
 * Resumen de datos reales del hardware, refrescados cada 1 s con lv_timer:
 *  - arcBattery / lblBattery : nivel de batería (ADC vía bsp_battery)
 *  - lblTemp                 : temperatura interna del chip (temperature_sensor)
 *  - lblUptime               : tiempo activo HH:MM:SS (esp_timer_get_time)
 *  - lblStatus               : indicador de estado (ícono + color) según la temperatura
 */
void scr_home_prepare(void);
void scr_home_init(void);
void scr_home_uninit(void);
void scr_home_step(void);

/** Acción del botón físico en esta pantalla: refresco inmediato. */
void scr_home_on_button(void);

#endif
