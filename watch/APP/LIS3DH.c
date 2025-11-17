



#include "LIS3DH.h"
#include "stm32f4xx_hal_i2c.h"
#include "string.h" // for memset
#include "string.h"
#include <stdint.h>
#include "i2c_dma_manager.h"

#include <arm_math.h>
#include <arm_const_structs.h>

#define LIS3DH_I2C_ADDRESS  (0x19 << 1) 

// LIS3DH 寄存器地址
#define LIS3DH_CTRL_REG1    (0x20)
#define LIS3DH_CTRL_REG4    (0x23)
#define LIS3DH_OUT_X_L      (0x28)
#define LIS3DH_READ_BURST   (LIS3DH_OUT_X_L | 0x80) // 0xA8

#define N       (64)                                              //采样个数
#define Fs      (10)                                              //采样频率
#define F_P     (((float)Fs)/N)
// 跌倒使能 计步使能 久坐使能 跌倒状态 久坐状态 计步数 久坐算法间隔比例 久坐时间
lis3dh lis3dhStruct = {1, 1, 1, 0, 0, 0, 10, 60};
LIS3DH_Data_t Accel_Data;
// 外部依赖的 I2C DMA 读写函数
extern osStatus_t I2C_Manager_Write_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout);
extern osStatus_t I2C_Manager_Read_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout);


/**
 * @brief LIS3DH 传感器初始化配置 (阻塞式)
 * * @param hi2c: I2C 句柄指针
 * @param timeout: HAL 阻塞超时时间 (毫秒)
 * @retval HAL_StatusTypeDef: HAL_OK 或错误代码
 */
HAL_StatusTypeDef LIS3DH_Init(I2C_HandleTypeDef *hi2c, uint32_t timeout)
{
    uint8_t tx_data;
    HAL_StatusTypeDef hal_status;

    // --- 1. 配置 CTRL_REG1 (0x20): ODR=50Hz, X/Y/Z 轴启用, Normal Mode (0x47) ---
    // 目的: 启用传感器，设置数据速率。
    tx_data = 0x47; 
    hal_status = HAL_I2C_Mem_Write(hi2c, 
                                   LIS3DH_I2C_ADDRESS, 
                                   LIS3DH_CTRL_REG1, 
                                   1, // 寄存器地址长度为 1 字节
                                   &tx_data, 
                                   1, // 数据长度为 1 字节
                                   timeout);

    // 检查第一次写入是否成功
    if (hal_status != HAL_OK)
    {
        return hal_status; // 如果失败，立即返回错误状态
    }
// --- 2. 配置 CTRL_REG4 (0x23): BDU 启用, Full Scale = +/-2g, High Resolution (0x88) ---
    // 目的: 启用 BDU 保证数据一致性，启用 High Resolution 获得 12-bit 精度，设置 +/-2g 量程。
    tx_data = 0x88; // BDU=1, FS=00 (+/-2g), HR=1
    hal_status = HAL_I2C_Mem_Write(hi2c, 
                                   LIS3DH_I2C_ADDRESS, 
                                   LIS3DH_CTRL_REG4, 
                                   1, 
                                   &tx_data, 
                                   1, 
                                   timeout);
    
    // 返回最终操作结果
    return hal_status;
}

// 2. 读取数据函数
static uint8_t buffer[6];
osStatus_t LIS3DH_Read_Acc_DMA(I2C_HandleTypeDef *hi2c, LIS3DH_Data_t *data, uint32_t timeout)
{
    memset(buffer, 0, sizeof(buffer));
    osStatus_t status;
    
    // **读取操作：**
    // MemAddress = LIS3DH_READ_BURST (0xA8)，表示从 0x28 开始连续读 6 个字节
    // HAL 库中，MemAddress 对应要读取的寄存器地址。
    status = I2C_Manager_Read_DMA(hi2c, 
                                LIS3DH_I2C_ADDRESS, 
                                LIS3DH_READ_BURST,  // 0xA8, 带有 MSB=1 的起始寄存器地址
                                1,                  // MemAddSize=1，表示寄存器地址是 1 个字节
                                buffer,             // 接收 X_L, X_H, Y_L, Y_H, Z_L, Z_H
                                6,                  // 读取 6 字节
                                timeout);

    if (status != osOK) return status;
// 1. 组合 16-bit 原始值 (小端模式) 并右移 4 位以提取 12-bit 有效数据。
    // 关键修复：先组合成 int16_t，再进行右移，确保负数符号位正确扩展。

    // X 轴
    int16_t raw_x_16bit = (int16_t)((buffer[1] << 8) | buffer[0]); // 组合成带符号的 16 位整数
    int16_t raw_x_shifted = raw_x_16bit >> 4;                       // 对带符号整数进行算术右移
    
    // Y 轴
    int16_t raw_y_16bit = (int16_t)((buffer[3] << 8) | buffer[2]);
    int16_t raw_y_shifted = raw_y_16bit >> 4; 
    
    // Z 轴
    int16_t raw_z_16bit = (int16_t)((buffer[5] << 8) | buffer[4]);
    int16_t raw_z_shifted = raw_z_16bit >> 4; 

    // 2. 转换为 g 值
    // 12-bit HR Mode, +/-2g 的灵敏度是 1000.0 LSB/g 
    #define LIS3DH_SENSITIVITY_2G_12BIT (1000.0f) 

    data->x_g = (float)raw_x_shifted / LIS3DH_SENSITIVITY_2G_12BIT;
    data->y_g = (float)raw_y_shifted / LIS3DH_SENSITIVITY_2G_12BIT;
    data->z_g = (float)raw_z_shifted / LIS3DH_SENSITIVITY_2G_12BIT;

    return osOK;


}



/*********************************************************************************************
* 名称：fallDect()
* 功能：跌倒检测处理算法
* 参数：x，y，z - 分别为三轴传感器的x，y，z值
* 返回：int - 跌倒状态
* 修改：
* 注释：
*********************************************************************************************/
int lis3dh_tumble(float x, float y, float z)
{
  float squareX = x * x;
  float squareY = y * y;
  float squareZ = z * z;
  float value;
  
  value = sqrt(squareX + squareY + squareZ);
  if (value < 5)
    return 1;
  return 0;
}


/*********************************************************************************************
* 名称：int stepcounting(float32_t* test_f32)
* 功能：步数计算函数
* 参数：
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
int stepcounting(float* test_f32)
{
  uint32_t ifftFlag = 0;                                        //傅里叶逆变换标志位
  uint32_t doBitReverse = 1;                                    //翻转标志位
  float testOutput[N/2];                                    //输出数组
  uint32_t i; 
  arm_cfft_f32(&arm_cfft_sR_f32_len64, test_f32, ifftFlag, doBitReverse);//傅里叶变换
  arm_cmplx_mag_f32(test_f32, testOutput, N/2);  
  float max = 0; 
  uint32_t mi = 0;  
  for (i=0; i<N/2; i++) {
    float a = testOutput[i];
    if (i == 0) a = testOutput[i]/(N);
    else a = testOutput[i]/(N/2);
    if (i != 0 && a > max && i*F_P <= 5.4f) {
        mi = i;
        max = a;
    }
  }
  if (max > 1.5) {
      int sc = 0;
      sc = (int)(mi * F_P * (1.0/Fs)*N);
      if (sc >= 3 && sc < 30) {
       return sc;
      }
  }
  return 0;
}

/*********************************************************************************************
* 名称：lis3dh_step()
* 功能：计步处理算法
* 参数：x，y，z - 分别为三轴传感器的x，y，z值
* 返回：
* 修改：
* 注释：
*********************************************************************************************/
void lis3dh_step(float x, float y, float z)
{
  static unsigned char tick = 0;
  static float acc_input[64*2];
  static unsigned short acc_len = 0;
  static unsigned char step_cnt = 0;
  float a = sqrt(x*x + y*y + z*z);
  acc_input[acc_len * 2] = a;
  acc_input[acc_len*2+1] = 0;
  acc_len++;
  if(acc_len == 64)
    acc_len = 0;
  if(acc_len == 0)
    step_cnt += stepcounting(acc_input);
  tick++;
  if(tick == lis3dhStruct.SedentaryInterval)
  {
    tick = 0;
    lis3dhStruct.stepCount += step_cnt;
    step_cnt = 0;
  }
}

/*********************************************************************************************
* 名称：lis3dh_SedentaryStatus()
* 功能：久坐处理算法
* 参数：x，y，z - 分别为三轴传感器的x，y，z值
* 返回：int - 久坐状态
* 修改：
* 注释：
*********************************************************************************************/
int lis3dh_sedentary(float x, float y, float z)
{
  static float lastX = 0, lastY = 0, lastZ = 9;
  static unsigned char count = 0;
  static unsigned short time = 0;
  if((x - 2 > lastX || x + 2 < lastX) 
     || (y - 2 > lastY || y + 2 < lastY) 
       || (z - 2 > lastZ || z + 2 < lastZ))
  {
    time = 0;
    count = 0;
  }
  else
    count++;
  if(count > lis3dhStruct.SedentaryInterval)
  {
	count= 0;
    time++;
  }
  if(time >= lis3dhStruct.SedentaryTime)
    return 1;
  else
    return 0;
}

/*********************************************************************************************
* 名称：run_lis3dh_arithmetic()
* 功能：运行lis3dh算法
* 参数：无
* 返回：int - 1/-1 执行成功/未开启算法使能
* 修改：
* 注释：
*********************************************************************************************/
int run_lis3dh_arithmetic(void)
{
  if(lis3dhStruct.tumbleEnableStatus || lis3dhStruct.stepEnableStatus 
     || lis3dhStruct.SedentaryEnableStatus)
  {
    float accX, accY, accZ;
    unsigned char runStatus = 0;
    accX=Accel_Data.x_g;
    accY=Accel_Data.y_g;
    accZ=Accel_Data.z_g;
    //get_lis3dhInfo(&accX, &accY, &accZ);
    if(lis3dhStruct.tumbleEnableStatus)
    {
      if(!(accX == 0 && accY == 0 && accZ == 0))
        lis3dhStruct.tumbleStatus = lis3dh_tumble(accX, accY, accZ);
      else
        lis3dhStruct.tumbleStatus = 0;
      runStatus |= 0x01;
    }
    if(lis3dhStruct.stepEnableStatus)
    {
      lis3dh_step(accX, accY, accZ);
      runStatus |= 0x02;
    }
    if(lis3dhStruct.SedentaryEnableStatus)
    {
      lis3dhStruct.SedentaryStatus = lis3dh_sedentary(accX, accY, accZ);
      runStatus |= 0x04;
    }
    return runStatus;
  }
  else
    return -1;
}

// 设置算法使能
void set_lis3dh_enableStatus(unsigned char cmd)
{
  if((cmd & 0x01) == 0x01)
    lis3dhStruct.tumbleEnableStatus = 1;
  else
    lis3dhStruct.tumbleEnableStatus = 0;
  if((cmd & 0x02) == 0x02)
    lis3dhStruct.stepEnableStatus = 1;
  else
    lis3dhStruct.stepEnableStatus = 0;
  if((cmd & 0x04) == 0x04)
    lis3dhStruct.SedentaryEnableStatus = 1;
  else
    lis3dhStruct.SedentaryEnableStatus = 0;
}

// 获取计步算法使能
unsigned char get_lis3dh_enableStatus(void)
{
  unsigned char status = 0;
  if(lis3dhStruct.tumbleEnableStatus)
    status |= 0x01;
  if(lis3dhStruct.stepEnableStatus)
    status |= 0x02;
  if(lis3dhStruct.SedentaryEnableStatus)
    status |= 0x04;
  return status;
}

// 获取当前计步数
int get_lis3dh_stepCount(void)
{
  if(lis3dhStruct.stepEnableStatus)
    return lis3dhStruct.stepCount;
  else
    return -1;
}

// 获取跌倒状态
int get_lis3dh_tumbleStatus(void)
{
  if(lis3dhStruct.tumbleEnableStatus)
    return lis3dhStruct.tumbleStatus;
  else
    return -1;
}

// 获取久坐状态
int get_lis3dh_SedentaryStatus(void)
{
  if(lis3dhStruct.SedentaryEnableStatus)
    return lis3dhStruct.SedentaryStatus;
  else
    return -1;
}

// 设置久坐时间系数
// 久坐算法运行间隔(单位：ms)，久坐时间(单位：s)
void set_lis3dh_SedentaryTime(unsigned short interval, unsigned short time)
{
  lis3dhStruct.SedentaryInterval = 1000 / interval;
  lis3dhStruct.SedentaryTime = time;
}

// 清空计步数
void del_lis3dh_stepCount(void)
{
  lis3dhStruct.stepCount = 0;
}
