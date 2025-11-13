#include "main.h"
#include "finger.h"
#include "uart_app.h"
#include "wk2xxx.h"

#include "cmsis_os2.h"
#include "GPS_Task.h"


void Task_Finger(void *argument)
{
    // 初始化指纹模块
    WK2114_init();
    vTaskDelay(100);
    Wk2114_SlaveRecv_Set(3, finger_input);
    vTaskDelay(10);

    set_fingerSend(Wk2114_Uart3SendByte);
    vTaskDelay(10);

    finger_init();
    vTaskDelay(10);
    GPS_Init();
    osDelay(1000);
    Wk2114_SlaveRecv_Set(1, gps_recv_ch);
    // 1. 发送 $CFGMSG,0,0,1*5C\r\n
    const char *cfg_msg_cmd = "$CFGMSG,0,0,1*5C\r\n";
    for (int i = 0; cfg_msg_cmd[i] != '\0'; i++) {
        Wk2114_Uart1SendByte(cfg_msg_cmd[i]);
    }
    
    // 2. 建议延时等待模块响应和处理
    osDelay(100);
    for (;;)
    {
        int state = finger_detect();
        if (state == 1) {

            char message[] = "Finger detected!\r\n";
            //Uart1_Tx.Uart_Send(&Uart1_Tx, (uint8_t*)message, strlen(message));
            finger_rgb_ctrl(LED_F_BLINK, LED_G,LED_B ,1 ); // 根据需要设置 LED
        } else if (state == 0) {

            char message[] = "No finger.\r\n";
            //Uart1_Tx.Uart_Send(&Uart1_Tx, (uint8_t*)message, strlen(message));
        }


  
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

