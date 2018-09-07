#include "dc_motor.h"
#include "global.h"
#include "UV.h"
#include "sys_run.h"

const unsigned char device_id = 1;

bit g_1ms_flag = 0;
bit g_2ms_flag = 0;
bit g_10ms_flag = 0;
bit g_100ms_flag = 0;
bit g_1s_flag = 0;
unsigned long g_1s_times = 0;
bit g_1min_flag = 0;
unsigned long g_sys_time_ms = 0;
//unsigned long g_sys_time_s = 0;


//风机反馈信号FG的计数
unsigned char FGcount = 0;


unsigned char speedBak = 0;	    //电机控制
bit gbMotorFGEn = 0;  //是否根据FG调节转速标志      
unsigned char gBMotorFGSet = 0;    
unsigned char	speed_dang = 0;
bit	sys_mode = 0; //系统运行模式，分为待机和运行	
bit	run_mode = 0; //运行模式分为自动和手动
bit gbMotorDelay = 0;
bit IsSpeedChanged = 0;  
bit IsFanRunning = 0;
unsigned int pwmDLbak = 0;     //当前PWM输出的占空比

signed long beats = 0;

unsigned int gBuzzerTimes_cnt = 0;
unsigned int gBuzzerEdge_cnt = 0;  //该变量调节模拟PWM的占空比


//硬件UART0
unsigned char U0RxBuffer[U0RxBuff_MAXSIZE] = {0};
unsigned char U0RxPtr = 0;

#ifdef WIFI_SOFT_UART
//模拟UART2
unsigned char U2RxBuffer[U2RxBuff_MAXSIZE];
unsigned char U2RxPtr = 0;
unsigned char U2TxBuffer[U2TxBuff_MAXSIZE];
unsigned char U2TxPtr = 0;
unsigned char U2LdPtr = 0;
unsigned char U2TxBitCount = 0;
unsigned char U2TxRate = 0;
//IsU2TxBusy = 1时，表示模拟UART2正在发送数据，如果有数据要发送，发送缓冲区有数据，发送的数据不能放在发送缓冲区buffer第0个字节，而是放在正在发送的数据后面
bit IsU2TxBusy = 0;
bit IsU2RxBusy = 0;
#endif

//模拟UART3
unsigned char U3RxBuffer[U3RxBuff_MAXSIZE];
unsigned char U3RxPtr = 0;
unsigned char U3TxBuffer[U3TxBuff_MAXSIZE];
unsigned char U3TxPtr = 0;
unsigned char U3LdPtr = 0;
unsigned char U3TxBitCount = 0;
unsigned char U3TxRate = 0;
//IsU2TxBusy = 1时，表示模拟UART2正在发送数据，如果有数据要发送，发送缓冲区有数据，发送的数据不能放在发送缓冲区buffer第0个字节，而是放在正在发送的数据后面
bit IsU3TxBusy = 0; 
bit IsU3RxBusy = 0;

bit IsSysFault = 0;
bit IsStepMotorBusy = 0;

//UV
bit IsUVOn = 0;
bit IsUVfault = 0;
//UV灯检测时间间隔10分钟，每次启动UV灯10秒后检测第一次，以防止UV灯有问题，以后每隔10分钟检测一次
//每次UV启动是必须设置 uv_check_timeinterval = UV_CHECK_TIME_INTERVAL - 10,在UV_On()函数中
unsigned int uv_check_timeinterval = UV_CHECK_TIME_INTERVAL - 10;
bit IsUVCheck = 0;
bit uv_check_flag = 0;
//UV反馈电压检测：每个一段时间连续检测3秒，每秒检测一次，间隔时间在UV.h中的 UV_CHECK_TIME_INTERVAL 宏定义
//每次刚上电时，第一次检测到的ADC的值为0，所以初始化为-1，上电后最开始检测4次，忽略第1次，以后每次都是检测3次
char uv_check_times = -1; 

//负氧离子工作状态
bit Is_ION_On = 0;

//仓门打开标志，次标志是扫码时下发的打开仓门标志
bit Is_Door_Open = 0;
//打开仓门的时间
unsigned long door_open_time = 0;

//传感器数据
float PM25_value = 0;
float PM25_value_bak = 0; //用于记录开机时PM2.5的数据
//float PM1_value = 0;
//float PM10_value = 0;
//float HCHO_value = 0;


//系统自检标志位
bit Is_selfcheck = 0;


//系统启动时间，用于处理传感器数据
unsigned long sys_start_time = 0;
//系统关闭时间，用于处理传感器数据
unsigned long sys_stop_time = 0;


unsigned int display_pm_value = 0;



//wifi控制MCU系统自检标志位
bit is_sys_auto_check = 0;
bit is_auto_check_complete = 0;
//wifi控制MCU系统自检标志位
bit is_sys_manual_check = 0;

sys_check_t sys_check_info;








