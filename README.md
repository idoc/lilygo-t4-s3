# BSP: LilyGo T4 S3

| [HW Reference](https://lilygo.cc/en-us/products/t4-s3) | [HOW TO USE API](API.md) | [EXAMPLES](#compatible-bsp-examples) | [![Component Registry](https://components.espressif.com/components/idoc/lilygo-t4-s3/badge.svg)](https://components.espressif.com/components/idoc/lilygo-t4-s3) | ![maintenance-status](https://img.shields.io/badge/maintenance-actively--developed-brightgreen.svg) |
| --- | --- | --- |-----------------------------------------------------------------------------------------------------------------------------------------------------------| -- |

## Overview

This is a Board Support Package for the LilyGo T4 S3. 

## Capabilities and dependencies

| Available          | Capability                             | Controller/Codec | Component                                                                                                 |
|--------------------|----------------------------------------|------------------|-----------------------------------------------------------------------------------------------------------|
| :heavy_check_mark: | :pager: DISPLAY                        | RM690B0          | [idoc/esp_lcd_panel_rm690b0](https://components.espressif.com/components/idoc/esp_lcd_panel_rm690b0)      |
| :heavy_check_mark: | :black_circle: LVGL_PORT (LVGL 9 only) |                  | [espressif/esp_lvgl_port](https://components.espressif.com/components/espressif/esp_lvgl_port)            |
| :heavy_check_mark: | :point_up: TOUCH                       | CST226SE         | [idoc/esp_lcd_touch_cst226se](https://components.espressif.com/components/idoc/esp_lcd_touch_cst226se) |

## Configuration Options

There are Kconfig options to select screen rotation, pixel bit depth (16bpp by default), as well as some pin selections. Screen rotation is probably the only one you'll need to examine. All options are under the "Board Support Package" section.

By default, a small DMA-capable buffer is created for LVGL. I find that this gives the best performance, which makes a noticeable difference with so many pixels on the screen.  
You can override these choices by calling `bsp_display_start_with_config()` instead of `bsp_display_start()`. Use the code in `bsp_display_start()` as an example to start with.

## Compatible BSP Examples

| Example                                                                                                    | Description                                                        |
|------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------|
| [Display Example](https://github.com/espressif/esp-bsp/tree/master/examples/display)                       | Show an image on the screen with a simple startup animation (LVGL) |
| [LVGL Benchmark Example](https://github.com/espressif/esp-bsp/tree/master/examples/display_lvgl_benchmark) | Run LVGL benchmark tests                                           |
| [LVGL Demos Example](https://github.com/espressif/esp-bsp/tree/master/examples/display_lvgl_demos)         | Run the LVGL demo player - all LVGL examples are included (LVGL)   |

## LVGL Benchmark

| Name | Avg. CPU | Avg. FPS | Avg. time | render time | flush time |
| ---- | :------: | :------: | :-------: | :---------: | :--------: |
| Empty screen | 96%  | 37  | 22  | 6  | 16  |
| Moving wallpaper | 98%  | 38  | 23  | 11  | 12  |
| Single rectangle | 30%  | 97  | 1  | 0  | 1  |
| Multiple rectangles | 90%  | 58  | 13  | 8  | 5  |
| Multiple RGB images | 24%  | 89  | 1  | 1  | 0  |
| Multiple ARGB images | 33%  | 90  | 4  | 3  | 1  |
| Rotated ARGB images | 82%  | 58  | 16  | 16  | 0  |
| Multiple labels | 56%  | 89  | 5  | 5  | 0  |
| Screen sized text | 97%  | 23  | 41  | 40  | 1  |
| Multiple arcs | 27%  | 90  | 0  | 0  | 0  |
| Containers | 38%  | 80  | 16  | 11  | 5  |
| Containers with overlay | 94%  | 32  | 26  | 22  | 4  |
| Containers with opa | 53%  | 77  | 17  | 14  | 3  |
| Containers with opa_layer | 51%  | 69  | 20  | 17  | 3  |
| Containers with scrolling | 97%  | 30  | 29  | 24  | 5  |
| Widgets demo | 99%  | 25  | 22  | 21  | 1  |
| All scenes avg. | 66%  | 61  | 15  | 12  | 3  |



<!-- END_BENCHMARK -->