#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "string.h"

/* This is taken straight from LVGL docs: 
https://docs.lvgl.io/master/integration/rtos/freertos.html
returns current CPU Usage. replaces the built in function in LVGL library

NOT USED YET!
*/
uint32_t lv_os_get_idle_percent(void);