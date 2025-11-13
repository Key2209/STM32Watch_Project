//#include "GPS_Task.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "main.h"
#include "queue.h"
#include "task.h"
#include "wk2xxx.h"
#include <stdio.h>
#include <string.h>
#include "math.h"
// 工具函数：

// #define fabs(x) __ev_fabs(x)

// INLINE static double __ev_fabs(double x) { return x > 0.0 ? x : -x; }

static int outOfChina(double lat, double lng) {
  if (lng < 72.004 || lng > 137.8347) {
    return 1;
  }
  if (lat < 0.8293 || lat > 55.8271) {
    return 1;
  }
  return 0;
}

#define EARTH_R 6378137.0

void transform(double x, double y, double *lat, double *lng) {
  double xy = x * y;
  double absX = sqrt(fabs(x));
  double xPi = x * M_PI;
  double yPi = y * M_PI;
  double d = 20.0 * sin(6.0 * xPi) + 20.0 * sin(2.0 * xPi);

  *lat = d;
  *lng = d;

  *lat += 20.0 * sin(yPi) + 40.0 * sin(yPi / 3.0);
  *lng += 20.0 * sin(xPi) + 40.0 * sin(xPi / 3.0);

  *lat += 160.0 * sin(yPi / 12.0) + 320 * sin(yPi / 30.0);
  *lng += 150.0 * sin(xPi / 12.0) + 300.0 * sin(xPi / 30.0);

  *lat *= 2.0 / 3.0;
  *lng *= 2.0 / 3.0;

  *lat += -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * xy + 0.2 * absX;
  *lng += 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * xy + 0.1 * absX;
}

static void delta(double lat, double lng, double *dLat, double *dLng) {
  if ((dLat == NULL) || (dLng == NULL)) {
    return;
  }
  const double ee = 0.00669342162296594323;
  transform(lng - 105.0, lat - 35.0, dLat, dLng);
  double radLat = lat / 180.0 * M_PI;
  double magic = sin(radLat);
  magic = 1 - ee * magic * magic;
  double sqrtMagic = sqrt(magic);
  *dLat = (*dLat * 180.0) / ((EARTH_R * (1 - ee)) / (magic * sqrtMagic) * M_PI);
  *dLng = (*dLng * 180.0) / (EARTH_R / sqrtMagic * cos(radLat) * M_PI);
}

void wgs2gcj(double wgsLat, double wgsLng, double *gcjLat, double *gcjLng) {
  if ((gcjLat == NULL) || (gcjLng == NULL)) {
    return;
  }
  if (outOfChina(wgsLat, wgsLng)) {
    *gcjLat = wgsLat;
    *gcjLng = wgsLng;
    return;
  }
  double dLat, dLng;
  delta(wgsLat, wgsLng, &dLat, &dLng);
  *gcjLat = wgsLat + dLat;
  *gcjLng = wgsLng + dLng;
}

void gcj2wgs(double gcjLat, double gcjLng, double *wgsLat, double *wgsLng) {
  if ((wgsLat == NULL) || (wgsLng == NULL)) {
    return;
  }
  if (outOfChina(gcjLat, gcjLng)) {
    *wgsLat = gcjLat;
    *wgsLng = gcjLng;
    return;
  }
  double dLat, dLng;
  delta(gcjLat, gcjLng, &dLat, &dLng);
  *wgsLat = gcjLat - dLat;
  *wgsLng = gcjLng - dLng;
}

void gcj2wgs_exact(double gcjLat, double gcjLng, double *wgsLat,
                   double *wgsLng) {
  double initDelta = 0.01;
  double threshold = 0.000001;
  double dLat, dLng, mLat, mLng, pLat, pLng;
  dLat = dLng = initDelta;
  mLat = gcjLat - dLat;
  mLng = gcjLng - dLng;
  pLat = gcjLat + dLat;
  pLng = gcjLng + dLng;
  int i;
  for (i = 0; i < 30; i++) {
    *wgsLat = (mLat + pLat) / 2;
    *wgsLng = (mLng + pLng) / 2;
    double tmpLat, tmpLng;
    wgs2gcj(*wgsLat, *wgsLng, &tmpLat, &tmpLng);
    dLat = tmpLat - gcjLat;
    dLng = tmpLng - gcjLng;
    if ((fabs(dLat) < threshold) && (fabs(dLng) < threshold)) {
      return;
    }
    if (dLat > 0) {
      pLat = *wgsLat;
    } else {
      mLat = *wgsLat;
    }
    if (dLng > 0) {
      pLng = *wgsLng;
    } else {
      mLng = *wgsLng;
    }
  }
}

double distance(double latA, double lngA, double latB, double lngB) {
  double arcLatA = latA * M_PI / 180;
  double arcLatB = latB * M_PI / 180;
  double x = cos(arcLatA) * cos(arcLatB) * cos((lngA - lngB) * M_PI / 180);
  double y = sin(arcLatA) * sin(arcLatB);
  double s = x + y;
  if (s > 1) {
    s = 1;
  }
  if (s < -1) {
    s = -1;
  }
  double alpha = acos(s);
  return alpha * EARTH_R;
}

const double x_pi = 3.14159265358979324 * 3000.0 / 180.0;

void gcj2bd(double gcjLat, double gcjLng, double *bdLat, double *bdLng) {
  double x = gcjLng, y = gcjLat;
  double z = sqrt(x * x + y * y) + 0.00002 * sin(y * x_pi);
  double theta = atan2(y, x) + 0.000003 * cos(x * x_pi);
  *bdLng = z * cos(theta) + 0.0065;
  *bdLat = z * sin(theta) + 0.006;
}

void bd2gcj(double bdLat, double bdLng, double *gcjLat, double *gcjLng) {
  double x = bdLng - 0.0065, y = bdLat - 0.006;
  double z = sqrt(x * x + y * y) - 0.00002 * sin(y * x_pi);
  double theta = atan2(y, x) - 0.000003 * cos(x * x_pi);
  *gcjLng = z * cos(theta);
  *gcjLat = z * sin(theta);
}

void wgs2bd(double wgsLat, double wgsLng, double *bdLat, double *bdLng) {
  double gcjLat, gcjLng;
  wgs2gcj(wgsLat, wgsLng, &gcjLat, &gcjLng);
  gcj2bd(gcjLat, gcjLng, bdLat, bdLng);
}

void bd2wgs(double bdLat, double bdLng, double *wgsLat, double *wgsLng) {
  double gcjLat, gcjLng;
  bd2gcj(bdLat, bdLng, &gcjLat, &gcjLng);
  gcj2wgs(gcjLat, gcjLng, wgsLat, wgsLng);
}







/////////////////////////////////////////////////////////////////

// NMEA 信息结构体 (简化)
typedef struct 
{ 
    double lat;         // 纬度 (DDMM.MMMM 格式，例如 3121.7247)
    double lon;         // 经度 (DDDMM.MMMM 格式，例如 12015.6887)
    int sig;            // 信号质量：1=GPS 定位，2=差分定位
    // ... 其他 NMEA 字段，如高度、卫星数等
} nmeaINFO; 

// NMEA 解析器结构体 (简化)
typedef struct 
{ 
    int state;
    // ... 解析时需要的内部状态变量
} nmeaPARSER;              

/**
 * @brief NMEA 库函数：清零 GPS 信息结构体
 * @param info 指向 nmeaINFO 结构体的指针
 */
void nmea_zero_INFO(nmeaINFO *info)
{
    memset(info, 0, sizeof(nmeaINFO));
}

/**
 * @brief NMEA 库函数：初始化解析器
 * @param parser 指向 nmeaPARSER 结构体的指针
 */
void nmea_parser_init(nmeaPARSER *parser)
{
    memset(parser, 0, sizeof(nmeaPARSER));
    parser->state = 0;
}

/**
 * @brief NMEA 库函数：将 DDMM.MMMM 格式转换为 DDD.DDDDDD 格式 (度分转十进制度)
 * @param ndeg DDMM.MMMM 格式的度数 (例如 3121.7247)
 * @return double DDD.DDDDDD 格式的十进制度数 (例如 31.362078)
 */
double nmea_ndeg2degree(double ndeg)
{
    double deg = (int)(ndeg / 100);
    double min = ndeg - deg * 100;
    return deg + min / 60.0;
}

/**
 * @brief NMEA 库函数：解析 NMEA 语句 (GPGGA/GNGGA)
 * * 这是一个模拟函数，实际解析逻辑非常复杂。
 * @param parser 解析器句柄
 * @param buff 待解析的 NMEA 字符串
 * @param buff_size 字符串长度
 * @param info 解析结果输出结构体
 * @return int 成功解析并有数据返回 1，否则返回 0
 */
int nmea_parse(nmeaPARSER *parser, const char *buff, int buff_size, nmeaINFO *info)
{
    // *** 实际项目中：此处应包含校验和检查、GPGGA字段提取和数值转换的复杂逻辑 ***
    if (buff_size < 15 || buff[0] != '$') return 0;

    // 模拟解析成功并填充数据 (假设数据是有效的)
    info->sig = 1;              // 模拟：有 GPS 信号
    info->lat = 3121.7247;      // 模拟：31度 21.7247分
    info->lon = 12015.6887;     // 模拟：120度 15.6887分
    
    return 1; // 模拟解析成功
}

/**
 * @brief 坐标系转换：WGS84 转换为 BD09 (百度坐标系)
 * * 这是一个模拟函数，实际转换涉及复杂的非线性算法。
 * @param wgs_lat WGS84 纬度
 * @param wgs_lon WGS84 经度
 * @param bd_lat BD09 纬度输出
 * @param bd_lon BD09 经度输出
 */
// void wgs2bd(double wgs_lat, double wgs_lon, double *bd_lat, double *bd_lon)
// {
//     // 模拟简单的偏移 (实际应使用专业的转换算法，如火星坐标GCJ02作为中间过渡)
//     *bd_lat = wgs_lat + 0.005;
//     *bd_lon = wgs_lon + 0.005;
// }












/////////////////////////////////////////////////
// FreeRTOS 队列句柄：用于传递“数据已准备好”的信号
QueueHandle_t xGpsQueue = NULL;
// 任务句柄，用于向 GPS_Task 发送通知
TaskHandle_t xGpsTaskHandle = NULL;


// NMEA 接收缓冲区长度
#define NMEA_RECV_LEN 128
// GPS 坐标存储：WGS84 转换为 BD09 后的字符串，精度 6 位小数
static char latitude[12] = {0}, longitude[12] = {0};
// 全局 NMEA GPGGA 语句缓冲区。回调函数将完整语句拷贝到此，线程读取。
static char nmea_gpgga[NMEA_RECV_LEN];

/* ============================ 接收数据回调函数 (ISR 安全)
 * ============================ */
/**
 * @brief GPS 串口接收回调函数 (WK2114 子串口 1)
 * * 此函数在 wk2xxx 驱动任务中被调用，属于普通任务上下文（非 ISR）。
 * 职责：1. 状态机捕获完整 $GPGGA/$GNGGA 语句；2. 将语句拷贝到全局缓冲区；3.
 * 通过非阻塞方式向 FreeRTOS 队列发送一个 uint8_t 信号。
 * * @param ch 接收到的单个字符
 */
void gps_recv_ch(char ch) {
  // 使用静态变量保持状态和临时接收缓冲区
  static char nmea_recv[NMEA_RECV_LEN];
  static uint16_t state = 0, index = 0;

  nmea_recv[index++] = ch;

  // 缓冲区溢出检查
  if (index >= NMEA_RECV_LEN) {
    state = 0;
    index = 0;
    return;
  }

  switch (state) {
  case 0: // 等待起始符 '$'
    if (ch == '$') {
      state = 1;
    } else {
      index = 0; // 不是起始符，重置索引
    }
    break;

  case 1: // 识别语句类型 (检查是否为 GPGGA/GNGGA)
    if (index == 6) {
      if (strncmp(nmea_recv, "$GPGGA", 6) == 0 ||
          strncmp(nmea_recv, "$GNGGA", 6) == 0) {
        state = 2; // 匹配成功
      } else {
        state = 0;
        index = 0; // 不是目标语句，重置
      }
    }
    break;

  case 2: // 等待回车符 '\r'
    if (ch == '\r') {
      state = 3;
    }
    break;

  case 3: // 等待换行符 '\n' (完整帧结束)
    if (ch == '\n') {
      // 1. 拷贝数据到全局缓冲区 nmea_gpgga
      nmea_recv[index] = '\0';
      memcpy(nmea_gpgga, nmea_recv, index + 1);

      // 2. 向 FreeRTOS 队列发送 uint8_t 信号
      uint8_t signal = 1;
// 2. ***** 替换队列发送：使用任务通知 *****
            if (xGpsTaskHandle != NULL) 
            {
                // 使用 xTaskNotifyGive 替换 xQueueSend(..., 0)
                // 这是一个非阻塞调用，且专门优化了任务间通信
                xTaskNotifyGive(xGpsTaskHandle);
            }


      // 3. 重置状态机
      state = 0;
      index = 0;
      return;
    }
    break;

  default:
    state = 0;
    index = 0;
    break;
  }
}

void GPS_Task(void *parameter) {
  (void)parameter;

  osDelay(5000); // 等待系统稳定
//   Wk2114_SlaveRecv_Set(1, gps_recv_ch);

  uint8_t signal; // 用于接收队列信号
  double deg_lat; // 转换成[degree].[degree]格式的纬度
  double deg_lon; // 转换成[degree].[degree]格式的经度

  // NMEA 解析库的数据结构
  // 假设这些结构体已在外部声明或头文件中包含
  nmeaINFO info;
  nmeaPARSER parser;

  nmea_zero_INFO(&info);     // 清零 GPS 信息结构体
  nmea_parser_init(&parser); // 初始化解析器

  for (;;) {
// pdTRUE 表示在接收到通知后，自动清除通知计数值
        uint32_t ulReceivedValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
        
        // 成功接收到信号 (ulReceivedValue > 0)
        if (ulReceivedValue > 0) 
        {
// 读取全局变量 nmea_gpgga 进行解析
            if (nmea_parse(&parser, (const char *)nmea_gpgga, strlen(nmea_gpgga), &info) > 0)
            {
                // 检查信号质量：1=GPS 定位, 2=差分 GPS 定位
                if (info.sig == 1 || info.sig == 2)
                {
                    // 1. 坐标格式转换：DDMM.MMMM -> DDD.DDDDDD (十进制)
                    deg_lat = nmea_ndeg2degree(info.lat);
                    deg_lon = nmea_ndeg2degree(info.lon);
                    
                    // 2. 坐标系转换：WGS84 -> 百度坐标 (BD09)
                    double bd_lat, bd_lon;
                    wgs2bd(deg_lat, deg_lon, &bd_lat, &bd_lon);
                    
                    // 3. 存储最终结果 (%.6f 格式化为 6 位小数的字符串)
                    sprintf(latitude, "%.6f", bd_lat);
                    sprintf(longitude, "%.6f", bd_lon);
                }
                else
                {
                    // 信号不好（无定位），清空坐标字符串
                    latitude[0] = '\0';
                    longitude[0] = '\0';
                }
            }
        }
        else // 接收超时 (pdFAIL/pdFALSE)
        {
            // 串口数据可能丢失，尝试重置子串口 1
            // printf("gps uart missed! reset gps uart...\n");
            // Wk2114PortInit(1);
            // Wk2114SetBaud(1, 9600);
        }
  }
}
int GPS_Init(void)
{
    // ***** 注意：xGpsTaskHandle 需要在此处获取句柄 *****
    // 假设您有一个创建 FreeRTOS 任务的函数
    BaseType_t res = xTaskCreate(
        GPS_Task, 
        "GPS_Task", 
        1024,   
        NULL, 
        7,      // 确保优先级合理
        &xGpsTaskHandle // 任务句柄存储到全局变量
    );

    if (res != pdPASS)
    {
        return -1; // 任务创建失败
    }
    
    return 0;
}
