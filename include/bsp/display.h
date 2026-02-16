/**
 * @file
 * @brief BSP LCD
 *
 * This file offers API for basic LCD control.
 * It is useful for users who want to use the LCD without the default Graphical Library LVGL.
 *
 * For standard LCD initialization with LVGL graphical library, you can call all-in-one function bsp_display_start().
 */

#pragma once
#include "esp_lcd_types.h"
#include "esp_err.h"

/** \addtogroup g04_display
 *  @{
 */

/* LCD resolution (when the screen is not rotated) */
#define BSP_LCD_H_HW_RES              (450)
#define BSP_LCD_V_HW_RES              (600)

#if defined(CONFIG_BSP_SCREEN_90_ROTATION) || defined(CONFIG_BSP_SCREEN_270_ROTATION)
#define BSP_LCD_H_RES                 (BSP_LCD_V_HW_RES)
#define BSP_LCD_V_RES                 (BSP_LCD_H_HW_RES)
#define BSP_LCD_SWAP_XY               (1)

#ifdef CONFIG_BSP_SCREEN_270_ROTATION
#define BSP_LCD_MIRROR_X              (0)
#define BSP_LCD_MIRROR_Y              (1)
#else
#define BSP_LCD_MIRROR_X              (1)
#define BSP_LCD_MIRROR_Y              (0)
#endif

#else
#define BSP_LCD_H_RES                 (BSP_LCD_H_HW_RES)
#define BSP_LCD_V_RES                 (BSP_LCD_H_RES)
#define BSP_LCD_SWAP_XY               (0)
#define BSP_LCD_MIRROR_X              (0)
#define BSP_LCD_MIRROR_Y              (0)
#endif

#ifdef __cplusplus
extern "C" {

#endif

/**
 * @brief BSP display configuration structure
 *
 */
typedef struct { // NOLINT(*-use-using)
    int max_transfer_sz; /*!< Maximum transfer size, in bytes. */
} bsp_display_config_t;

/**
 * @brief Create new display panel
 *
 * For maximum flexibility, this function performs only reset and initialization of the display.
 * You must turn on the display explicitly by calling esp_lcd_panel_disp_on_off().
 * The display's backlight is not turned on either. You can use bsp_display_backlight_on/off(),
 * bsp_display_brightness_set() (on supported boards) or implement your own backlight control.
 *
 * If you want to free resources allocated by this function, you can use esp_lcd API, i.e.:
 *
 * \code{.c}
 * esp_lcd_panel_del(panel);
 * esp_lcd_panel_io_del(io);
 * spi_bus_free(spi_num_from_configuration);
 * \endcode
 *
 * @param[in]  config    display configuration
 * @param[out] ret_panel esp_lcd panel handle
 * @param[out] ret_io    esp_lcd IO handle
 * @return
 *      - ESP_OK         On success
 *      - Else           esp_lcd failure
 */
esp_err_t bsp_display_new(const bsp_display_config_t* config, esp_lcd_panel_handle_t* ret_panel,
                          esp_lcd_panel_io_handle_t* ret_io);

/**
 * @brief Initialize display's brightness (does nothing in this implementation)
 *
 * In an LCD, brightness is usually controlled with PWM signal to a pin controlling backlight.
 * AMOLED displays have no backlight and don't need this function. It's left here for compatibility with other BSPs.
 *
 * @return
 *      - ESP_OK                On success
 */
esp_err_t bsp_display_brightness_init(void);

/**
 * @brief Set display's brightness
 *
 * Unlike some LCDs, the AMOLED driver does not need to be initialized before setting brightness.
 * This esp_lcd driver used defaults to 100% brightness.
 *
 * @param[in] brightness_percent Brightness in [%]
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   Parameter error
 */
esp_err_t bsp_display_brightness_set(int brightness_percent);

/**
 * @brief Set the display to full brightness
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   Parameter error
 */
esp_err_t bsp_display_backlight_on(void);


/**
 * @brief Turn the display brightness to 0
 *
 * Technically, this is different to turning the display off, but since this is AMOLED,
 * it will be completely dark and lok turned off.
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   Parameter error
 */
esp_err_t bsp_display_backlight_off(void);

#ifdef __cplusplus
}
#endif

/** @} */ // end of display
