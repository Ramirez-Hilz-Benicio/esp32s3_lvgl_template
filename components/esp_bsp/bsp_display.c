#include "bsp_display.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "bsp_display";

static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static uint8_t s_brightness = 0;

void bsp_display_init(void)
{
    ESP_LOGI(TAG, "SPI BUS init");
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = BSP_PIN_LCD_SCLK;
    buscfg.mosi_io_num = BSP_PIN_LCD_MOSI;
    buscfg.miso_io_num = BSP_PIN_LCD_MISO;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_DRAW_BUFF_HEIGHT;
    ESP_ERROR_CHECK(spi_bus_initialize(BSP_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = BSP_PIN_LCD_CS;
    io_config.dc_gpio_num = BSP_PIN_LCD_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_SPI_HOST, &io_config, &s_io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(s_io_handle, &panel_config, &s_panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));
}

esp_lcd_panel_io_handle_t bsp_display_get_io_handle(void)
{
    return s_io_handle;
}

esp_lcd_panel_handle_t bsp_display_get_panel_handle(void)
{
    return s_panel_handle;
}

void bsp_display_brightness_init(void)
{
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LCD_BL_LEDC_MODE;
    ledc_timer.timer_num = LCD_BL_LEDC_TIMER;
    ledc_timer.duty_resolution = LCD_BL_LEDC_DUTY_RES;
    ledc_timer.freq_hz = LCD_BL_LEDC_FREQUENCY;
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {};
    ledc_channel.speed_mode = LCD_BL_LEDC_MODE;
    ledc_channel.channel = LCD_BL_LEDC_CHANNEL;
    ledc_channel.timer_sel = LCD_BL_LEDC_TIMER;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.gpio_num = BSP_PIN_LCD_BL;
    ledc_channel.duty = 0;
    ledc_channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void bsp_display_set_brightness(uint8_t brightness)
{
    if (brightness > 100) {
        brightness = 100;
        ESP_LOGE(TAG, "Brightness value out of range");
    }
    s_brightness = brightness;
    uint32_t duty = (brightness * (LCD_BL_LEDC_DUTY - 1)) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL));
    ESP_LOGI(TAG, "LCD brightness set to %d%%", brightness);
}

uint8_t bsp_display_get_brightness(void)
{
    return s_brightness;
}

/* ------------------------------------------------------------------ */
/* Fade-in de retroiluminación con lv_anim + LEDC                      */
/* ------------------------------------------------------------------ */

/* exec_cb de la animación: aplica el brillo sin loguear en cada paso
 * (bsp_display_set_brightness() imprime un log por llamada).
 *
 * El ojo percibe el brillo de forma no lineal: con un duty lineal la
 * pantalla parece encenderse casi de golpe en los primeros ms. Se aplica
 * una curva cúbica (duty ∝ v³, aproximación de la corrección gamma) para
 * que el fade-in se perciba gradual durante toda su duración. */
static void fade_exec_cb(void *var, int32_t value)
{
    (void)var;
    if (value < 0)   value = 0;
    if (value > 100) value = 100;
    s_brightness = (uint8_t)value;
    uint32_t v    = (uint32_t)value;
    uint32_t duty = (v * v * v * (LCD_BL_LEDC_DUTY - 1)) / (100U * 100U * 100U);
    ledc_set_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL, duty);
    ledc_update_duty(LCD_BL_LEDC_MODE, LCD_BL_LEDC_CHANNEL);
}

void bsp_display_fade_in(uint32_t duration_ms)
{
    lv_anim_del(&s_brightness, fade_exec_cb);   /* por si ya había un fade en curso */

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &s_brightness);
    lv_anim_set_exec_cb(&a, fade_exec_cb);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);   /* la curva perceptual ya está en exec_cb */
    lv_anim_start(&a);
    ESP_LOGI(TAG, "backlight fade-in (%u ms)", (unsigned)duration_ms);
}
