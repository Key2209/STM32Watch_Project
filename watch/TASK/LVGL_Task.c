#include "cmsis_os2.h"
#include "freertos.h"
#include "main.h"
#include "misc/lv_timer.h"


#include "lvgl.h"                // 它为整个LVGL提供了更完整的头文件引用
#include "lv_port_disp.h"        // LVGL的显示支持
#include "lv_port_indev.h"       // LVGL的触屏支持
#include "lvgl/demos/widgets/lv_demo_widgets.h"  
#include "stdio.h"
void Task_LVGL(void *argument) {


    // 检查初始堆内存
    size_t free_heap = xPortGetFreeHeapSize();
    printf("[LVGL] Free heap before init: %d bytes\n", free_heap);
    
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    
    // 检查初始化后的堆内存
    free_heap = xPortGetFreeHeapSize();
    printf("[LVGL] Free heap after init: %d bytes\n", free_heap);
    
    lv_demo_widgets();
    
    // 检查Demo启动后的堆内存
    free_heap = xPortGetFreeHeapSize();
    printf("[LVGL] Free heap after demo: %d bytes\n", free_heap);
  for (;;) {

    lv_timer_handler();
    osDelay(pdMS_TO_TICKS(5));
  }
}
