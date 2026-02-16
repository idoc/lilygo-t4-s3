#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_spiffs.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#include "bsp/lilygo-t4-s3.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "esp_lcd_touch_cst226se.h"
#include "esp_lcd_rm690b0.h"
#include "esp_lvgl_port.h"
#include "bsp_err_check.h"
#include "esp_lcd_panel_interface.h"

// NOLINTBEGIN (*-avoid-non-const-global-variables)
static const char* TAG = "T4 S3";

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)

#ifdef CONFIG_BSP_LV_COLOR_FORMAT_RGB565
#define BSP_LCD_COLOR_FORMAT                (LV_COLOR_FORMAT_RGB565)
#define BSP_LCD_BITS_PER_PIXEL   (16)
#else
#define BSP_LCD_COLOR_FORMAT                (LV_COLOR_FORMAT_RGB888)
#define BSP_LCD_BITS_PER_PIXEL   (24)
#endif

static lv_display_t* lv_display = NULL;
static lv_indev_t* disp_indev_touch = NULL;
#endif // BSP_CONFIG_NO_GRAPHIC_LIB == 0
static esp_lcd_touch_handle_t tp; // LCD touch handle
static bool i2c_initialized = false;
static bool spi_initialized = false;

static esp_lcd_panel_t* lcd_panel = NULL;
/**
 * @brief I2C handle for BSP usage
 *
 * In IDF v5.4 you can call i2c_master_get_bus_handle(BSP_I2C_NUM, i2c_master_bus_handle_t *ret_handle)
 * from #include "esp_private/i2c_platform.h" to get this handle
 *
 * For IDF 5.2 and 5.3 you must call bsp_i2c_get_handle()
 */
static i2c_master_bus_handle_t i2c_handle = NULL;

// ReSharper disable once CppDFAConstantFunctionResult
esp_err_t bsp_i2c_init(void) {
    /* I2C was initialized before */
    if (i2c_initialized) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = BSP_I2C_NUM,
        .sda_io_num = BSP_I2C_SDA,
        .scl_io_num = BSP_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
            .allow_pd = false
        },
    };

    BSP_ERROR_CHECK_RETURN_ERR(i2c_new_master_bus(&i2c_config, &i2c_handle));

    i2c_initialized = true;
    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void) {
    BSP_ERROR_CHECK_RETURN_ERR(i2c_del_master_bus(i2c_handle));
    i2c_initialized = false;
    return ESP_OK;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(void) {
    bsp_i2c_init();
    return i2c_handle;
}

static esp_err_t bsp_spi_init(uint32_t max_transfer_sz) {
    /* SPI was initialized before */
    if (spi_initialized) {
        return ESP_OK;
    }

    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t bus_config = {
        .data0_io_num = BSP_LCD_DATA0,
        .data1_io_num = BSP_LCD_DATA1,
        .sclk_io_num = BSP_LCD_PCLK,
        .data2_io_num = BSP_LCD_DATA2,
        .data3_io_num = BSP_LCD_DATA3,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
    };

    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &bus_config, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    spi_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_spiffs_mount(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_BSP_SPIFFS_MOUNT_POINT,
        .partition_label = CONFIG_BSP_SPIFFS_PARTITION_LABEL,
        .max_files = CONFIG_BSP_SPIFFS_MAX_FILES,
#ifdef CONFIG_BSP_SPIFFS_FORMAT_ON_MOUNT_FAIL
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
    };

    esp_err_t ret_val = esp_vfs_spiffs_register(&conf);

    BSP_ERROR_CHECK_RETURN_ERR(ret_val);

    size_t total = 0, used = 0;
    ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret_val;
}

esp_err_t bsp_spiffs_unmount(void) {
    return esp_vfs_spiffs_unregister(CONFIG_BSP_SPIFFS_PARTITION_LABEL);
}

// Number of bits used to represent command and parameter
#define LCD_CMD_BITS           32
#define LCD_PARAM_BITS         8

esp_err_t bsp_display_brightness_init(void) {
    return ESP_OK;
}

esp_err_t bsp_display_brightness_set(int brightness_percent) {
    const uint8_t brightness = brightness_percent * 255 / 100;
    return esp_lcd_panel_rm690b0_set_brightness(lcd_panel, brightness);
}

esp_err_t bsp_display_backlight_off(void) {
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void) {
    return bsp_display_brightness_set(100); // NOLINT(*-avoid-magic-numbers)
}

esp_err_t bsp_display_new(const bsp_display_config_t* config, esp_lcd_panel_handle_t* ret_panel,
                          esp_lcd_panel_io_handle_t* ret_io) {
    esp_err_t ret = ESP_OK;
    assert(config != NULL && config->max_transfer_sz > 0);

    /* Initialize SPI */
    ESP_RETURN_ON_ERROR(bsp_spi_init(config->max_transfer_sz), TAG, "");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = -1,
        .cs_gpio_num = BSP_LCD_CS,
        .spi_mode = 0,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .trans_queue_depth = 10,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .flags = {
            .dc_high_on_cmd = 0,
            .dc_low_on_data = 0,
            .dc_low_on_param = 0,
            .octal_mode = 0,
            .quad_mode = 1,
            .sio_mode = 0,
            .lsb_first = 0,
            .cs_high_active = 0,
        }
    };

    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi(BSP_LCD_SPI_NUM, &io_config, ret_io), err, TAG,
                      "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    rm960b0_vendor_config_t vendor_config = {
        .en_gpio_num = BSP_LCD_EN,
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
        .vendor_config = &vendor_config,
    };

    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_rm690b0(*ret_io, &panel_config, ret_panel), err, TAG, "New panel failed");

    lcd_panel = *ret_panel;
    esp_lcd_panel_reset(*ret_panel);
    esp_lcd_panel_init(*ret_panel);
    return ret;

err:
    if (*ret_panel) {
        esp_lcd_panel_del(*ret_panel);
    }
    if (*ret_io) {
        esp_lcd_panel_io_del(*ret_io);
    }
    spi_bus_free(BSP_LCD_SPI_NUM);
    return ret;
}

esp_err_t bsp_touch_new(const bsp_touch_config_t* _, esp_lcd_touch_handle_t* ret_touch) {
    /* Initialize I2C */
    BSP_ERROR_CHECK_RETURN_ERR(bsp_i2c_init());

    /* Initialize touch */
    const esp_lcd_touch_config_t tp_config = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = BSP_LCD_TOUCH_RST,
        .int_gpio_num = BSP_LCD_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = BSP_LCD_SWAP_XY,
            .mirror_x = BSP_LCD_MIRROR_X,
            .mirror_y = BSP_LCD_MIRROR_Y,
        },
    };

    return esp_lcd_touch_new_i2c_cst226se(bsp_i2c_get_handle(), &tp_config, ret_touch);
}

static void lvgl_round_cb(lv_event_t* e) {
    lv_area_t* area = lv_event_get_param(e);
    if (area->x1 % 2) {
        area->x1--;
    }

    if (area->y1 % 2) {
        area->y1--;
    }

    if (lv_area_get_width(area) % 2) {
        area->x2++;
    }

    if (lv_area_get_height(area) % 2) {
        area->y2++;
    }
}

#if (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
static lv_display_t* bsp_display_lcd_init(const bsp_display_cfg_t* cfg) {
    assert(cfg != NULL);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = BSP_LCD_DRAW_BUFF_SIZE * (BSP_LCD_BITS_PER_PIXEL / 8),
    };
    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new(&bsp_disp_cfg, &panel_handle, &io_handle));

    esp_lcd_panel_disp_on_off(panel_handle, true);

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = cfg->buffer_size,
        .double_buffer = cfg->double_buffer,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = BSP_LCD_SWAP_XY,
            .mirror_x = BSP_LCD_MIRROR_X,
            .mirror_y = BSP_LCD_MIRROR_Y,
        },
        .color_format = BSP_LCD_COLOR_FORMAT,
        .flags = {
            .buff_dma = cfg->flags.buff_dma,
            .buff_spiram = cfg->flags.buff_spiram,
            .swap_bytes = false,
        }
    };

    lv_display = lvgl_port_add_disp(&disp_cfg);
    assert(lv_display);
    lv_display_add_event_cb(lv_display, lvgl_round_cb, LV_EVENT_INVALIDATE_AREA, lv_display);

    return lv_display;
}

static lv_indev_t* bsp_display_indev_touch_init(lv_display_t* disp) {
    BSP_ERROR_CHECK_RETURN_NULL(bsp_touch_new(NULL, &tp));
    assert(tp);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

lv_display_t* bsp_display_start(void) {
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
        }
    };

    return bsp_display_start_with_config(&cfg);
}

lv_display_t* bsp_display_start_with_config(const bsp_display_cfg_t* cfg) {
    assert(cfg != NULL);
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&cfg->lvgl_port_cfg));

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_brightness_init());

    BSP_NULL_CHECK((lv_display = bsp_display_lcd_init(cfg)), NULL);

    BSP_NULL_CHECK((disp_indev_touch = bsp_display_indev_touch_init(lv_display)), NULL);

    return lv_display;
}

bool bsp_display_lock(uint32_t timeout_ms) {
    return lvgl_port_lock(timeout_ms);
}

void bsp_display_unlock(void) {
    lvgl_port_unlock();
}
#endif // (BSP_CONFIG_NO_GRAPHIC_LIB == 0)
// NOLINTEND (*-avoid-non-const-global-variables)
