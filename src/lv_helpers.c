#include "lv_helpers.h"

uint32_t lv_os_get_idle_percent(void) {
    static uint32_t last_idle_time = 0;
    static uint32_t last_total_time = 0;

    uint32_t ulTotalRunTime;
    uint32_t ulIdleTime = 0;

    TaskStatus_t *pxTaskStatusArray;
    UBaseType_t uxArraySize, x;

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if(pxTaskStatusArray == NULL) {
        return 0;
    }

    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
    for(x = 0; x < uxArraySize; x++) {
        // ESP32 has IDLE0 and IDLE1 tasks
        if (strcmp(pxTaskStatusArray[x].pcTaskName, "IDLE0") == 0 ||
            strcmp(pxTaskStatusArray[x].pcTaskName, "IDLE1") == 0) {
            ulIdleTime += pxTaskStatusArray[x].ulRunTimeCounter;
        }
    }
    vPortFree(pxTaskStatusArray);

    uint32_t idle_diff = ulIdleTime - last_idle_time;
    uint32_t total_diff = ulTotalRunTime - last_total_time;

    last_idle_time = ulIdleTime;
    last_total_time = ulTotalRunTime;

    if(total_diff == 0) return 0;

    // combined idle percentage across both cores (0-200)
    uint32_t idle_pct = (idle_diff * 100) / total_diff;
    // return normalized load (0-100)
    return idle_pct / 2;
}