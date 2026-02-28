#pragma once

#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "lvgl.h"

/* macros for SSD1306 */
#define OLED_SDA_PIN        22
#define OLED_SCL_PIN        21
#define LCD_PIXEL_CLOCK_HZ  (400 * 1000)
#define OLED_I2C_ADDR       0x3C
#define OLED_H_RES          128
#define OLED_V_RES          64

void display_init(void);