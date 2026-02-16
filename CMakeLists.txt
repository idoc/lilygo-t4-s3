idf_component_register(
        SRCS "lilygo-t4-s3.c"
        INCLUDE_DIRS "include"
        PRIV_INCLUDE_DIRS "priv_include"

        REQUIRES
        esp_driver_i2c
        fatfs
        esp_lvgl_port

        PRIV_REQUIRES
        esp_lcd_rm690b0
        esp_lcd_touch_cst226se
        esp_lcd
        spiffs
        esp_psram
)
