#include "cmsis_os2.h"
#include "st7789.h"
#include "stm32f4xx_hal.h" 
#include <stdio.h>         

#ifdef HAL_SRAM_MODULE_ENABLED
#define DEBUG 0

// 简单的毫秒延时宏，替换 apl_delay/delay.h 中的 delay_ms
#define delay_ms(x) osDelay(x)

// 假设 FSMC_HandleTypeDef 已在 main.c 中定义和初始化
// extern SRAM_HandleTypeDef hsram; 

screen_dev_t screen_dev = {
  .wide = ST7789_WIDE,
  .high = ST7789_HIGH,
  .id = 0,
  .dir = 1,// 横屏
  .wramcmd = 0X2C,
  .rramcmd = 0X2E,
  .setxcmd = 0x2A,
  .setycmd = 0x2B,
};

// **重要说明：以下两个函数 ST7789_GPIO_Config 和 ST7789_FSMC_Config 已被移除或注释**
// **因为在CubeMX移植中，GPIO和FSMC的初始化应由CubeMX生成，并在main()函数中调用。**
// **您只需要确保CubeMX正确配置了FSMC及其时序。**

uint32_t St7789_IDGet()
{
    uint16_t v3, v4;
    write_command(0x04);
    // 读两次数据
    v3 = ST7789_DAT;
    v4 = ST7789_DAT;
    if (!((0x85==v3 && 0x52 == v4) || (v3==0x04 && v4==0x85))){
        // 移除 rt_kprintf
        // printf("error: can't fund lcd ic\r\n"); 
        return 0;
    }
    return 1;
}

/**
  * @brief  ST7789 初始化序列
  * @param  None
  * @retval 0: 成功, 1: 失败
  * @note   此函数假设 FSMC 和 GPIO 已经通过 CubeMX 配置并初始化。
  */
uint8_t St7789_init(void)
{
    // ST7789_GPIO_Config(); // 移除
    // ST7789_FSMC_Config(); // 移除

    // 1. 背光引脚初始化（如果CubeMX没有初始化，需要在此手动添加）
    // 为了简化，我们只在 st7789.h 中定义宏，假设引脚已初始化
    // 在 main.c 中调用 St7789_init() 之前，确保 MX_GPIO_Init() 和 MX_FSMC_Init() 已执行。

    if(St7789_IDGet() == 0)
        return 1;
    //delay_ms(120);  
                for (volatile uint32_t i = 0; i < 1680000; i++) {
        __NOP();  // 无操作指令，防止被优化
    }
    // 打开背光
    ST7789_Light_ON();

    // ST7789 初始化命令序列 (保留不变)
    write_command(0x11);
    delay_ms(120);                                                //Delay 120ms
    write_command(0x3A);
    write_data(0x05);

    write_command(0x36);
    if(screen_dev.dir)
      write_data(0xA0);
    else
      write_data(0x00);

// ... (ST7789S Frame rate, Power, Gamma setting command sequence, 保留不变)
#if 1
  //ST7789S Frame rate setting
  write_command(0xb2);
  write_data(0x00);
  write_data(0x00);
  write_data(0x00);
  write_data(0x33);
  write_data(0x33);
  write_command(0xb7);
  write_data(0x35);
  //ST7789S Power setting
  write_command(0xb8);
  write_data(0x2f);
  write_data(0x2b);
  write_data(0x2f);
  write_command(0xbb);
  write_data(0x24);                                             //vcom
  write_command(0xc0);
  write_data(0x2C);
  write_command(0xc3);
  write_data(0x10);                                             //0B调深浅
  write_command(0xc4);
  write_data(0x20);
  write_command(0xc6);
  write_data(0x11);
  write_command(0xd0);
  write_data(0xa4);
  write_data(0xa1);
  write_command(0xe8);
  write_data(0x03);
  write_command(0xe9);
  write_data(0x0d);
  write_data(0x12);
  write_data(0x00);
  //ST7789S gamma setting
  write_command(0xe0);
  write_data(0xd0);
  write_data(0x00);
  write_data(0x00);
  write_data(0x08);
  write_data(0x11);
  write_data(0x1a);
  write_data(0x2b);
  write_data(0x33);
  write_data(0x42);
  write_data(0x26);
  write_data(0x12);
  write_data(0x21);
  write_data(0x2f);
  write_data(0x11);
  write_command(0xe1);
  write_data(0xd0);
  write_data(0x02);
  write_data(0x09);
  write_data(0x0d);
  write_data(0x0d);
  write_data(0x27);
  write_data(0x2b);
  write_data(0x33);
  write_data(0x42);
  write_data(0x17);
  write_data(0x12);
  write_data(0x11);
  write_data(0x2f);
  write_data(0x31);
  write_command(0x21);                                          //反显
  write_command(0x29);                                          //display on
#else
  // ... (Alternative init sequence, 保留不变)
  write_command(0xb2);
  write_data(0x0c);
  write_data(0x0c);
  write_data(0x00);
  write_data(0x33);
  write_data(0x33);

  write_command(0xb7);
  write_data(0x35);

  write_command(0xbb);
  write_data(0X35);//vcom flick

  write_command(0xc2);
  write_data(0x01);
   
  write_command(0xc3);//gvdd
  write_data(0x10); 
                                                                                      
  write_command(0xc4);
  write_data(0x20);

  write_command(0xc6);
  write_data(0x0f);

  write_command(0xd0);
  write_data(0xa4);
  write_data(0xa2);
   
  write_command(0xe0);
  write_data(0xD0);
  write_data(0x00);
  write_data(0x02);
  write_data(0x07);
  write_data(0x0A);
  write_data(0x28);
  write_data(0x32);
  write_data(0x44);
  write_data(0x42);
  write_data(0x06);
  write_data(0x0E);
  write_data(0x12);
  write_data(0x14);
  write_data(0x17);

  write_command(0xe1);
  write_data(0xD0);
  write_data(0x00);
  write_data(0x02);
  write_data(0x07);
  write_data(0x0A);
  write_data(0x28);
  write_data(0x31);
  write_data(0x54);
  write_data(0x47);
  write_data(0x0E);
  write_data(0x1C);
  write_data(0x17);
  write_data(0x1B);
  write_data(0x1E);

  write_command(0x3a);
  write_data(0x55);

  write_command(0x2A); 
  write_data(0x00);
  write_data(0x00);
  write_data(0x00);
  write_data(0xEF);

  write_command(0x2B); 
  write_data(0x00);
  write_data(0x00);
  write_data(0x01);
  write_data(0x3F);
  //**********************
  write_command(0x11);
  delay_ms(120);      //Delay 120ms 
  write_command(0x29); //display on
  delay_ms(25);
  write_command(0x2c);
#endif
  
  // St7789_FillColor(0, 0, screen_dev.wide-1, screen_dev.high-1, 0x0000);
  // St7789_DrawPoint(120, 160, 0xF800);
  return 0;
}


/********************************************************************************************/
// 以下所有绘图函数（包括方向设置、写/读点、填充）都只依赖于 FSMC 宏和 HAL_Delay，**无需修改**。

uint8_t St7789_DirectionSet(uint8_t dir)
{
    if(dir>1) return 1;
    write_command(0x36);
    if(dir)
    {
      write_data(0xA0);
      screen_dev.wide = ST7789_WIDE;
      screen_dev.high = ST7789_HIGH;
    }
    else
    {
      write_data(0x00);
      screen_dev.wide = ST7789_HIGH;
      screen_dev.high = ST7789_WIDE;
    }
    screen_dev.dir = dir;
    return 0;
}

void St7789_PrepareWrite()
{
    write_command(screen_dev.wramcmd);
}

void St7789_SetCursorPos(short x,short y)
{
    write_command(screen_dev.setxcmd);                                         
    write_data(x>>8);
    write_data(x&0xff);
    write_command(screen_dev.setycmd);                                         
    write_data(y>>8);
    write_data(y&0xff);
}

void St7789_SetWindow(short x1, short y1, short x2, short y2)
{
    write_command(0x2A);                                          
    write_data(x1>>8);
    write_data(x1);
    write_data(x2>>8);
    write_data(x2);
    write_command(0x2B);                                          
    write_data(y1>>8);
    write_data(y1);
    write_data(y2>>8);
    write_data(y2);
}

void St7789_PrepareFill(short x1, short y1, short x2, short y2)
{
    St7789_SetWindow(x1,y1,x2,y2);
    St7789_PrepareWrite();
}

void St7789_DrawPoint(short x,short y,uint32_t color)
{
  write_command(0x2A);                                          
  write_data(x>>8);
  write_data(x&0xff);
  write_command(0x2B);                                          
  write_data(y>>8);
  write_data(y&0xff);

  write_command(0x2C);

  write_data(color);
}

uint32_t St7789_ReadPoint(short x,short y)
{
    uint32_t r=0,g=0,b=0;
    St7789_SetCursorPos(x,y);
    write_command(screen_dev.rramcmd);
    r = read_data();//dummy Read
    r = read_data();//RG
    b = read_data();//BR
    g = r&0X00FF;
    r = r&0XFF00;
    return (r|((g>>2)<<5)|(b>>11));
}

void St7789_FillColor(short x1,short y1,short x2,short y2,uint32_t color)
{
    uint16_t x, y;
    St7789_SetWindow(x1,y1,x2,y2);
    St7789_PrepareWrite();
    for (y=y1; y<=y2; y++)
    {
      for (x=x1; x<=x2; x++)
      {
        write_data(color);
      }
    }
}

void St7789_FillData(short x1,short y1,short x2,short y2,unsigned short* dat)
{
    short x, y;
    St7789_SetWindow(x1,y1,x2,y2);
    St7789_PrepareWrite();
    for (y=y1; y<=y2; y++)
    {
      for (x=x1; x<=x2; x++)
      {
        write_data(*dat);
        dat++;
      }
    }
}



#endif //#ifdef HAL_SRAM_MODULE_ENABLED

