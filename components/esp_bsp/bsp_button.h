#ifndef __BSP_BUTTON_H__
#define __BSP_BUTTON_H__

#include "esp_err.h"
#include "driver/gpio.h"

/* Botón físico BOOT de la placa (activo en bajo, con pull-up interno). */
#define BSP_PIN_BUTTON            GPIO_NUM_0
#define BSP_BUTTON_ACTIVE_LEVEL   0

#ifdef __cplusplus
extern "C" {
#endif

/** Callback de click. Se ejecuta en la tarea de esp_timer del componente
 *  espressif/button: NO debe llamar funciones de LVGL. */
typedef void (*bsp_button_click_cb_t)(void);

/**
 * Registra el botón físico con el componente `espressif/button` y llama a
 * `on_click` en cada pulsación simple (BUTTON_SINGLE_CLICK).
 */
esp_err_t bsp_button_init(bsp_button_click_cb_t on_click);

#ifdef __cplusplus
}
#endif

#endif
