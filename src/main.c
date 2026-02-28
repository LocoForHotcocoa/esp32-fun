#include "display.h"

void app_main(void) {
    display_init();

    // draw a simple label
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello ESP32!");
    lv_obj_center(label);
}