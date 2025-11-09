#include "PCF8563.h"
#include "i2c_dma_manager.h"
// -----------------------------------------------------
// BCD 转换函数
// -----------------------------------------------------

/**
 * @brief 将 BCD 码转换为十进制数
 * @param bcd_val 要转换的 BCD 值
 * @return 转换后的十进制值
 * @explanation
 * BCD 码的高 4 位 (高半字节) 是十位 (tens)，低 4 位 (低半字节) 是个位 (units)。
 * (bcd_val >> 4) * 10 计算十位的值。
 * (bcd_val & 0x0F) 获取个位的值。两者相加即为结果。
 */
uint8_t BCD_to_DEC(uint8_t bcd_val) 
{
    return ((bcd_val >> 4) * 10) + (bcd_val & 0x0F);
}

/**
 * @brief 将十进制数转换为 BCD 码 (用于写操作)
 * @param dec_val 要转换的十进制值
 * @return 转换后的 BCD 值
 */
uint8_t DEC_to_BCD(uint8_t dec_val)
{
    // (dec_val / 10) 获取十位，(dec_val % 10) 获取个位
    // (dec_val / 10) << 4 将十位移到高 4 位
    return ((dec_val / 10) << 4) + (dec_val % 10);
}

// -----------------------------------------------------
// PCF8563 初始化与设置 (写入操作示例)
// -----------------------------------------------------

/**
 * @brief PCF8563 初始化（设置控制寄存器）
 * @param hi2c I2C 句柄指针
 * @return HAL_StatusTypeDef 状态
 * @explanation
 * 1. 控制寄存器 1 (0x00)：Bit 5 (STOP) 为 0，表示时钟振荡器运行。
 * 我们写入 0x00，确保时钟正常启动。
 */
HAL_StatusTypeDef PCF8563_Init(I2C_HandleTypeDef *hi2c) 
{
    // 配置 Control_status_1 寄存器 (0x00)，写入 0x00，确保时钟晶振运行 (STOP=0)。
    uint8_t ctrl_data = 0x00; 

    // 使用 HAL_I2C_Mem_Write 函数：
    // 1. hi2c: I2C 句柄
    // 2. PCF8563_DEV_ADDR: 0xA2 (设备地址)
    // 3. PCF8563_REG_CONTROL1: 0x00 (寄存器地址)
    // 4. I2C_MEMADD_SIZE_8BIT: 寄存器地址宽度为 8 位
    // 5. &ctrl_data: 要写入的数据
    // 6. 1: 写入 1 个字节
    // 7. 100: 超时时间 100ms
    return HAL_I2C_Mem_Write(
        hi2c, 
        PCF8563_DEV_ADDR, 
        PCF8563_REG_CONTROL1, 
        I2C_MEMADD_SIZE_8BIT, 
        &ctrl_data, 
        1, 
        100
    );
}

/**
 * @brief 设置 PCF8563 的初始时间/日期
 * @param hi2c I2C 句柄指针
 * @param init_time 要设置的时间/日期结构体
 * @return HAL_StatusTypeDef 状态
 * @explanation
 * 1. 构造一个 7 字节的 BCD 格式数据数组。
 * 2. 使用 HAL_I2C_Mem_Write 一次性写入 7 个寄存器。
 */
osStatus_t PCF8563_Set_Time(I2C_HandleTypeDef *hi2c, PCF8563_Time_t *init_time)
{
    uint8_t tx_buf[PCF8563_REG_WRITE_COUNT];
    osStatus_t status;
    // 将十进制数据转换为 BCD 格式并存储到发送缓冲区
    tx_buf[0] = DEC_to_BCD(init_time->seconds) & 0x7F; // 秒 (Bit 7: VL位需清零或忽略)
    tx_buf[1] = DEC_to_BCD(init_time->minutes) & 0x7F; // 分
    tx_buf[2] = DEC_to_BCD(init_time->hours)   & 0x3F; // 时 (Bit 6,7 是 x)
    tx_buf[3] = DEC_to_BCD(init_time->day)     & 0x3F; // 日
    tx_buf[4] = DEC_to_BCD(init_time->weekday) & 0x07; // 星期
    tx_buf[5] = DEC_to_BCD(init_time->month)   & 0x1F; // 月 (Bit 7 是世纪位，暂不处理)
    tx_buf[6] = DEC_to_BCD(init_time->year);           // 年

    // 使用 HAL_I2C_Mem_Write 一次性写入 7 个字节
    status=I2C_Manager_Write_DMA(
        hi2c,
        PCF8563_DEV_ADDR,
        PCF8563_REG_VL_SECONDS, // 起始寄存器地址 0x02
        I2C_MEMADD_SIZE_8BIT,
        tx_buf,
        PCF8563_REG_WRITE_COUNT, // 写入 7 个字节
        1000
    );

    return status;
}

// -----------------------------------------------------
// PCF8563 读取操作 (核心功能)
// -----------------------------------------------------

/**
 * @brief 从 PCF8563 读取实时时间/日期
 * @param hi2c I2C 句柄指针
 * @param time 用于存储读取到的时间/日期结构体
 * @return HAL_StatusTypeDef 状态
 * @explanation
 * 1. 使用 HAL_I2C_Mem_Read 从 0x02 地址开始读取 7 个字节。
 * PCF8563 支持寄存器地址自动递增，因此一次调用即可读取所有时间信息。
 * 2. 对读取到的 BCD 原始数据进行位掩码 (mask) 清除标志位。
 * 3. 调用 BCD_to_DEC 函数将 BCD 码转换为十进制数，存入结构体。
 */
osStatus_t PCF8563_Read_Time(I2C_HandleTypeDef *hi2c, PCF8563_Time_t *time) 
{
    uint8_t rx_buf[PCF8563_REG_WRITE_COUNT];
    osStatus_t status;

    // --- 第 1 步：使用 HAL_I2C_Mem_Read 读取原始数据 ---
    // [1] I2C 通信：从设备 0xA2 的内存地址 0x02 开始，读取 7 个字节
    status = I2C_Manager_Read_DMA(
        hi2c, 
        0xA3, 
        PCF8563_REG_VL_SECONDS, // 起始寄存器地址 0x02PCF8563_REG_VL_SECONDS
        I2C_MEMADD_SIZE_8BIT,   // 寄存器地址宽度
        rx_buf,                 // 接收缓冲区
        PCF8563_REG_WRITE_COUNT,// 读取 7 个字节 (秒到年)
        500                   // 超时时间
    );

    if (status != osOK)
    {
        return status; // 读取失败
    }

    // --- 第 2 步：位掩码和 BCD 转换 ---
    
    // PCF8563 数据手册规定了每个寄存器中有效 BCD 数据的位范围，需要用位掩码 (AND操作) 清除高位标志位。
    
    // 秒：0x02 寄存器，Bits 0-6 是秒数据，Bit 7 是 VL 标志位 (Voltage Low)。
    // 掩码 0x7F (0b01111111) 清除 Bit 7，保留 BCD 数据。
    time->seconds = BCD_to_DEC(rx_buf[0] & 0x7F);

    // 分钟：0x03 寄存器，Bits 0-6 是分钟数据。
    time->minutes = BCD_to_DEC(rx_buf[1] & 0x7F);
    
    // 小时：0x04 寄存器，Bits 0-5 是小时数据。
    // 掩码 0x3F (0b00111111) 清除 Bits 6-7。
    time->hours   = BCD_to_DEC(rx_buf[2] & 0x3F);
    
    // 日：0x05 寄存器，Bits 0-5 是日数据。
    time->day     = BCD_to_DEC(rx_buf[3] & 0x3F);
    
    // 星期：0x06 寄存器，Bits 0-2 是星期数据 (0-6)。
    time->weekday = BCD_to_DEC(rx_buf[4] & 0x07);
    
    // 月：0x07 寄存器，Bits 0-4 是月数据。
    time->month   = BCD_to_DEC(rx_buf[5] & 0x1F); // Bit 7 是世纪标志 (Century)，Bit 5-6 是 x
    
    // 年：0x08 寄存器，Bits 0-7 是年数据 (后两位)。
    time->year    = BCD_to_DEC(rx_buf[6]);

    return status;
}
