/*********************************************************************************************
* 文件：ft6x36.c (HAL 移植版)
* 说明：FT6x36 触摸屏驱动程序 (基于 STM32 HAL 库)
*********************************************************************************************/
#include "ft6x36.h"
#include "cmsis_os2.h"
// #include <cstddef>
#include <string.h>
#include "i2c_dma_manager.h"
#include "ST7789.h"
#include "uart_app.h"
// 假设我们使用 I2C1 句柄，它在 main.c/i2c.c 中初始化
extern I2C_HandleTypeDef hi2c1;

// 调试宏（可选，可以替换为您的 printf 或 log 系统）
#define FT6X36_DEBUG 0
#if FT6X36_DEBUG
#include <stdio.h>
#define UART_DEBUG(...) printf(__VA_ARGS__)
#else
#define UART_DEBUG(...)
#endif

// 触摸中断回调函数指针
void (*TouchIrq)(void) = NULL;

TOUCH_Dev_t TouchDev = {
    .direction = 1,
    .wide = 320,
    .high = 240,
    .id = 0,
};

/*********************************************************************************************
* 名称：ft6x36_write_reg()
* 功能：FT6x36 写寄存器 (HAL I2C 实现)
* 参数：reg_addr -- 寄存器地址
* value    -- 要写入的值
* 返回：0-成功，其他-失败
*********************************************************************************************/
static int ft6x36_write_reg(uint8_t reg_addr, uint8_t value)
{
HAL_StatusTypeDef status;

    // 使用 HAL_I2C_Mem_Write
    // MemAddress = reg_addr, pData = &value, Size = 1
    status = HAL_I2C_Mem_Write(&hi2c1, 
                               (FT6X36_IIC_ADDR << 1), // 8位设备地址
                               reg_addr,               // 寄存器地址
                               I2C_MEMADD_SIZE_8BIT,   // 寄存器地址大小 (1字节)
                               &value,                 // 要写入的数据指针 (只传一个字节)
                               1,                      // 写入的数据长度 (1字节)
                               HAL_MAX_DELAY);

    if (status != HAL_OK)
    {
        // UART_DEBUG("FT6x36 Write Reg Error!\r\n"); // 启用您的调试输出
        return -1;
    }
    return 0;
}

/*********************************************************************************************
* 名称：ft6x36_read_reg()
* 功能：FT6x36 读寄存器 (HAL I2C 实现)
* 参数：reg_addr -- 寄存器地址
* 返回：读取到的 8 位数据 (或 -1 失败)
*********************************************************************************************/
static int ft6x36_read_reg(uint8_t reg_addr)
{
    uint8_t value = 0;
    HAL_StatusTypeDef status;

    // 使用 HAL_I2C_Mem_Read 替代手动分步的 Master_Transmit + Master_Receive
    status = HAL_I2C_Mem_Read(&hi2c1, 
                              (FT6X36_IIC_ADDR << 1),  // 8位设备地址 (0x70)
                              reg_addr,                // 寄存器地址 (0xA8)
                              I2C_MEMADD_SIZE_8BIT,    // 寄存器地址大小 (1字节)
                              &value,                  // 接收缓冲区
                              1,                       // 接收长度 (1字节)
                              HAL_MAX_DELAY);          // 阻塞等待

    if (status != HAL_OK)
    {
        // I2C 物理传输失败
        return -1;
    }

    // 物理传输成功，返回读取到的数据
    return value;
}

static int ft6x36_read_reg_DMA(uint8_t reg_addr)
{
    uint8_t value = 0;
    osStatus_t status;

    // FT6x36 的寄存器地址大小是 8 位，MemAddSize 设为 I2C_MEMADD_SIZE_8BIT
    status = I2C_Manager_Read_DMA(&hi2c1, 
                              (FT6X36_IIC_ADDR << 1),  // 8位设备地址
                              reg_addr,                // 寄存器地址
                              I2C_MEMADD_SIZE_8BIT,    // 寄存器地址大小 (1字节)
                              &value,                  // 接收数据的缓冲区
                              1,                       // 接收数据的长度 (1字节)
                              500);          // 阻塞等待

    if (status != osOK)
    {
        // 您可以在这里添加错误码判断，例如：
        // if (hi2c1.ErrorCode & HAL_I2C_ERROR_AF)
        // { /* 应答失败 */ } 
        
        UART_DEBUG("FT6x36 Read Reg Error (Mem_Read)!\r\n");
        return -1;
    }

    return value;
}
/*********************************************************************************************
* 名称：ft6x36_read_len()
* 功能：FT6x36 连续读取多个寄存器 (HAL I2C 实现)
* 参数：reg_addr -- 起始寄存器地址
* buff     -- 接收缓冲区
* len      -- 接收长度
* 返回：0-成功，其他-失败
*********************************************************************************************/
static int ft6x36_read_len(uint8_t reg_addr, uint8_t *buff, uint16_t len)
{
    uint8_t value = 0;
    HAL_StatusTypeDef status;

    // 使用 HAL_I2C_Mem_Read 替代手动分步的 Master_Transmit + Master_Receive
    status = HAL_I2C_Mem_Read(&hi2c1, 
                              (FT6X36_IIC_ADDR << 1),  // 8位设备地址 (0x70)
                              reg_addr,                // 寄存器地址 (0xA8)
                              I2C_MEMADD_SIZE_8BIT,    // 寄存器地址大小 (1字节)
                              &value,                  // 接收缓冲区
                              len,                       // 接收长度 (1字节)
                              HAL_MAX_DELAY);          // 阻塞等待

    if (status != HAL_OK)
    {
        // I2C 物理传输失败
        return -1;
    }

    // 物理传输成功，返回读取到的数据
    return value;
}
static int ft6x36_read_len_DMA(uint8_t reg_addr, uint8_t *buff, uint16_t len)
{
    // // 1. 发送起始寄存器地址 (写操作)
    // if (I2C_Manager_Write_DMA(&hi2c1, (FT6X36_IIC_ADDR << 1), &reg_addr, 1, HAL_MAX_DELAY) != HAL_OK)
    // {
    //     UART_DEBUG("FT6x36 Read Len (Addr) Error!\r\n");
    //     return -1;
    // }
    
    // // 2. 连续读取数据 (读操作)
    // if (I2C_Manager_Read_DMA(&hi2c1, (FT6X36_IIC_ADDR << 1), buff, len, HAL_MAX_DELAY) != HAL_OK)
    // {
    //     UART_DEBUG("FT6x36 Read Len (Data) Error!\r\n");
    //     return -1;
    // }
    
    // return 0;
    uint8_t value = 0;
    osStatus_t status;

    // FT6x36 的寄存器地址大小是 8 位，MemAddSize 设为 I2C_MEMADD_SIZE_8BIT
    status = I2C_Manager_Read_DMA(&hi2c1, 
                              (FT6X36_IIC_ADDR << 1),  // 8位设备地址
                              reg_addr,                // 寄存器地址
                              I2C_MEMADD_SIZE_8BIT,    // 寄存器地址大小 (1字节)
                              &buff,                  // 接收数据的缓冲区
                              len,                       // 接收数据的长度 (1字节)
                              500);          // 阻塞等待

    if (status != osOK)
    {
        // 您可以在这里添加错误码判断，例如：
        // if (hi2c1.ErrorCode & HAL_I2C_ERROR_AF)
        // { /* 应答失败 */ } 
        
        //UART_DEBUG("FT6x36 Read Reg Error (Mem_Read)!\r\n");
        return -1;
    }


}
/*********************************************************************************************
* 名称：TouchIrqSet()
* 功能：设置触摸中断触发的回调函数
* 参数：func -- 回调函数指针
* 注释：在 FreeRTOS 中，此函数通常用于设置任务通知或信号量释放。
*********************************************************************************************/
void TouchIrqSet(void (*func)(void))
{
    TouchIrq = func;
}

/*********************************************************************************************
* 名称：ft6x36_Exti_Callback()
* 功能：FT6x36 中断处理回调函数 (替代 RT-Thread IRQ)
* 注释：此函数应在 CubeMX 生成的 GPIO_EXTI_IRQHandler 中被调用。
*********************************************************************************************/
void ft6x36_Exti_Callback(void)
{
    if (TouchIrq != NULL)
    {
        TouchIrq(); // 调用用户设置的回调函数
    }
}

void Touch_Callback();



/*********************************************************************************************
* 名称：ft6x36_init()
* 功能：触摸屏初始化 (替代 RT-Thread I2C/PIN 初始化)
* 返回：0-成功，其他-失败
*********************************************************************************************/
int ft6x36_init(void)
{
    int r;
    
// 1. RST 引脚拉低，进入复位状态
    FT6X36_RESET_L();
            for (volatile uint32_t i = 0; i < 1680000; i++) {
        __NOP();  // 无操作指令，防止被优化
    }

    // 2. RST 引脚拉高，退出复位状态
    FT6X36_RESET_H();
           for (volatile uint32_t i = 0; i < 1680000; i++) {
        __NOP();  // 无操作指令，防止被优化
    }
    // 【RT-Thread 延时替换为 HAL_Delay】
    //osDelay(10); // 等待芯片上电稳定
        for (volatile uint32_t i = 0; i < 1680000; i++) {
        __NOP();  // 无操作指令，防止被优化
    }
    // 1. 读取芯片 ID 0xA8
    r = ft6x36_read_reg(FT_REG_CHIPID);
    if (r < 0)
    {
        //UART_DEBUG("error: ft6x36 read chip ID failed\r\n");
        return 1;
    }
    
    // 2. 检查芯片 ID (FT6236 ID 通常是 0x23)
    if (!(r == 0x11 || r == 0x23 || r == 0x51)) // 常见 ID：FT6236=0x23, FT6336=0x51
    {
        //UART_DEBUG("error: can't find ic ft6x36, ID: 0x%X\r\n", r);
        return 2;
    }  
    
    // 存储 ID
    TouchDev.id = r;
    
    // 3. 配置 FT6x36 寄存器（通常使用默认配置即可，这里保留原代码的配置）
    ft6x36_write_reg(0, 0);       // Mode: 正常工作模式
    ft6x36_write_reg(0x80, 22);   // 阈值 (Threshold for touch detection)
    ft6x36_write_reg(0x88, 12);   // 滤波系数
    
    // 4. 初始化触摸屏尺寸
    TouchDev.wide = screen_dev.wide;
    TouchDev.high = screen_dev.high;
    
    // 注意：GPIO 中断的初始化 (HAL_GPIO_Init) 应由 CubeMX 完成
    // 只需要确保 CubeMX 中断优先级和中断服务函数正确配置即可。
    
TouchIrqSet(Touch_Callback); // 设置中断回调函数
    return 0;
}

/*********************************************************************************************
* 名称：ft6x36_TouchScan()
* 功能：扫描并读取触摸点数据
*********************************************************************************************/
void ft6x36_TouchScan(void)
{
    // uint8_t touch_data[8];
    // uint8_t status;
    // uint8_t i;
    
    // // 1. 读取触摸点状态 (寄存器 0x02)
    // status = ft6x36_read_reg_DMA(FT_REG_P_STATUS);
    
    // // 2. 获取触摸点数量 (Status)
    // TouchDev.status = status & 0x0F;
    // if (TouchDev.status > TOUCH_NUM)
    // {
    //     TouchDev.status = TOUCH_NUM;
    // }

    // if (TouchDev.status > 0)
    // {
    //     // 3. 读取触摸点 1 的坐标 (寄存器 0x03 到 0x08，共 6 字节)
    //     // 0x03(P1_XH), 0x04(P1_XL), 0x05(P1_YH), 0x06(P1_YL), 0x07(P2_XH), 0x08(P2_XL)
    //     // FT6x36 的触摸点信息是从寄存器 0x03 开始的连续 6 或 8 个字节
    //     if (ft6x36_read_len_DMA(FT_REG_P1_XH, touch_data, 8) == 0) // 读取 8 字节 (P1 和 P2 的 X/Y)
    //     {
    //         for (i = 0; i < TouchDev.status; i++)
    //         {
    //             // X 坐标：高 4 位在 XH 寄存器，低 8 位在 XL 寄存器
    //             TouchDev.x[i] = ((touch_data[i * 6 + 0] & 0x0F) << 8) | touch_data[i * 6 + 1];
                
    //             // Y 坐标：高 4 位在 YH 寄存器，低 8 位在 YL 寄存器
    //             TouchDev.y[i] = ((touch_data[i * 6 + 2] & 0x0F) << 8) | touch_data[i * 6 + 3];
                
    //             // 坐标映射和边界检查（根据屏幕方向和实际连接进行调整）
    //             if (TouchDev.x[i] >= TouchDev.wide)  TouchDev.x[i] = TouchDev.wide - 1;
    //             if (TouchDev.y[i] >= TouchDev.high) TouchDev.y[i] = TouchDev.high - 1;
    //             //St7789_DrawPoint(TouchDev.x[0], TouchDev.y[0], 0xF800); // 红色
    //             St7789_FillColor(TouchDev.x[0], TouchDev.y[0], TouchDev.x[0]+20, TouchDev.y[0]+20, 0xF800); 
    //         }
    //     }
    // }
    // 否则，status 为 0，表示没有触摸





      uint8_t i=0;
  int status = ft6x36_read_reg_DMA(2);
  //if(status > TOUCH_NUM) status = TOUCH_NUM;
  if(status > 0 && status<=TOUCH_NUM)
  {
    TouchDev.status = status;
    
    for(i=0;i<status;i++)
    {
      TouchDev.x[i]=(ft6x36_read_reg_DMA(3+(6*i))&0x0f);
      TouchDev.x[i]<<=8;
      TouchDev.x[i]+=ft6x36_read_reg_DMA(4+(6*i));
      
      TouchDev.y[i]=(ft6x36_read_reg_DMA(5+(6*i))&0x0f);
      TouchDev.y[i]<<=8;
      TouchDev.y[i]+=ft6x36_read_reg_DMA(6+(6*i));
    }
    
    if(TouchDev.direction)
    {
      uint16_t temp=0;
      for(i=0;i<TOUCH_NUM;i++)
      {
        temp = TouchDev.y[i];
        TouchDev.y[i] = TouchDev.x[i];
        TouchDev.x[i] = TouchDev.wide-temp;
      }
    }      

    //printf("Touch X:%d Y:%d\r\n",TouchDev.x[0],TouchDev.y[0]);

  }
  else
  {
    for(i=0;i<TOUCH_NUM;i++)
    {
      TouchDev.x[i] = -1;
      TouchDev.y[i] = -1;
    }
  }
}



void Touch_Callback()
{
    // ft6x36_TouchScan();

    // if (TouchDev.x!=NULL && TouchDev.y!=NULL)
    // {
    //     St7789_DrawPoint(TouchDev.x[0], TouchDev.y[0], 0xF800); // 红色
    // }
    
    //Uart1_Tx.Uart_Send(&Uart1_Tx, (uint8_t *)"Touch IRQ Triggered!\r\n", 23);


}
