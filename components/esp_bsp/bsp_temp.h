#ifndef __BSP_TEMP_H__
#define __BSP_TEMP_H__

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Inicializa y habilita el sensor de temperatura interno del ESP32-S3
 * (driver `temperature_sensor` de ESP-IDF). Llamar una sola vez en app_main().
 */
esp_err_t bsp_temp_sensor_init(void);

/**
 * Lee la temperatura del chip en grados Celsius.
 * Devuelve ESP_ERR_INVALID_STATE si el sensor no fue inicializado.
 */
esp_err_t bsp_temp_sensor_read(float *celsius);

#ifdef __cplusplus
}
#endif

#endif
