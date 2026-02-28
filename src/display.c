#include "display.h"

static const char* TAG = "OLED_APP";

// To use LV_COLOR_FORMAT_I1, we need an extra buffer to hold the converted data
static uint8_t oled_buffer[OLED_H_RES * OLED_V_RES / 8];

// Callback: tells LVGL the I2C transfer is finished
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void lvgl_port_task(void *arg) {
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms < 10) sleep_ms = 10;
        vTaskDelay(pdMS_TO_TICKS(sleep_ms));
    }
}

// Callback: packs bits for the SSD1306 ?
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);

    // LVGL 9 adds an 8 byte palette header for monochrome
    px_map += 8;

    int x1 = area->x1;
    int x2 = area->x2;
    int y1 = area->y1;
    int y2 = area->y2;

    // loop coverts LVGL's pixel format to SSD1306's page format
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            bool color = (px_map[(OLED_H_RES >> 3) * y + (x >> 3)] & (1 << (7 - x % 8)));
            uint8_t *buf_ptr = &oled_buffer[OLED_H_RES * (y >> 3) + x];
            if (color) (*buf_ptr) &= ~(1 << (y % 8)); // pixel OFF (inverse logic usually)
            else       (*buf_ptr) |=  (1 << (y % 8)); // pixel ON
        }
    }
    // send the converted buffer to the hardware
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, oled_buffer);
}

void display_init(void) {
    ESP_LOGI(TAG, "Starting OLED Setup...");

    // 1. setup I2C
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = OLED_SCL_PIN,
        .sda_io_num = OLED_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    // 2. create the panel IO
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = OLED_I2C_ADDR,
        .scl_speed_hz = LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6, // specific to SSD1306
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    // 3. Create panel driver
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1, // set to gpio if you have a reset pin
    };

    // SSD1306 specific config (technically not needed here since default is 128x64)
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = OLED_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));
    // 4. power it on
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "OLED initialized successfully!");

    // 5. initialize LVGL core
    lv_init();

    // 6. create the LVGL display object
    lv_display_t* disp = lv_display_create(OLED_H_RES, OLED_V_RES);
    lv_display_set_user_data(disp, panel_handle);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1); // monochrome

    // 7. set the draw buffer
    static uint8_t draw_buf[OLED_H_RES * OLED_V_RES / 8 + 8];
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_FULL);

    // 8. set callbacks
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    const esp_lcd_panel_io_callbacks_t io_cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &io_cbs, disp);

    // 9. start LVGL task
    xTaskCreate(lvgl_port_task, "LVGL", 4096, NULL, 5, NULL);
}