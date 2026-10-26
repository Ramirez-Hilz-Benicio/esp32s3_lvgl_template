#include "bsp_temp.h"

#include "driver/temperature_sensor.h"
#include "esp_log.h"

static const char *TAG = "bsp_temp";

static temperature_sensor_handle_t s_temp_handle = NULL;

esp_err_t bsp_temp_sensor_init(void)
{
    /* Rango esperado de medición: -10 °C a 80 °C. El driver elige
     * internamente el rango de hardware con mejor precisión para ese intervalo. */
    temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);

    esp_err_t err = temperature_sensor_install(&cfg, &s_temp_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "install failed: %s", esp_err_to_name(err));
        s_temp_handle = NULL;
        return err;
    }

    err = temperature_sensor_enable(s_temp_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "enable failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "temperature sensor ready");
    return ESP_OK;
}

esp_err_t bsp_temp_sensor_read(float *celsius)
{
    if (s_temp_handle == NULL || celsius == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return temperature_sensor_get_celsius(s_temp_handle, celsius);
}
