/*--------------------------------------------------------------------------
global.H

 extern global resource for C51.
--------------------------------------------------------------------------*/
#ifndef		_GLOBAL_H
#define		_GLOBAL_H

#include "touch_key.h"
#include "SOFT_IIC.h"
#include "common.h"
#include "charge.h"
#include "M26.h"
#include "sys_run.h"


/*------------------------------------------------------------------------------------------
;								datastruct.c
;-----------------------------------------------------------------------------------------*/

//----------------------------计沮挡篶--------------------------------------------------------
//typedef	unsigned char	Byte;
//typedef	unsigned int	Word;
//typedef	unsigned long	Dword;
////////////////////////////////////////////////////////////
//typedef unsigned char BYTE;
//typedef unsigned int  WORD;
//typedef unsigned long DWORD;

//typedef unsigned char uchar;
//typedef unsigned int  uint;
//typedef unsigned long ulong;

//typedef unsigned char u8;
//typedef unsigned char U8;
//typedef unsigned int u16;
//typedef unsigned int  U16;

//#define	uCHAR	unsigned char
//#define	sCHAR	signed   char
//#define	uINT	unsigned short
//#define	sINT	signed   short
//#define	uLONG	unsigned long
//#define	sLONG	signed long
//#define	npage	xdata
//#define	zpage	data
////////////////////////////////////////////////////

// mode SYS
#define  				STANDBY				0 //待机
#define  				RUNNING				1 //工作中



#define _test_timeflag(x)    ((x == 1)?1:0) 



//硬件UART0接收数据缓冲区数组长度定义
#define U0RxBuff_MAXSIZE   155
extern char idata U0RxBuffer[U0RxBuff_MAXSIZE];
extern unsigned char idata U0RxPtr;


//模拟UART2接收数据缓冲区数组长度定义
#ifdef WIFI_SOFT_IIC
#define U2RxBuff_MAXSIZE   100
extern unsigned char U2RxBuffer[U2RxBuff_MAXSIZE];
extern unsigned char U2RxPtr;
#endif

#ifdef WIFI_SOFT_IIC
#define U2TxBuff_MAXSIZE   100
extern unsigned char U2TxBuffer[U2TxBuff_MAXSIZE];
extern unsigned char U2TxPtr;
extern unsigned char U2LdPtr;
extern unsigned char U2TxBitCount;
extern unsigned char U2TxRate;
extern bit IsU2TxBusy;
extern bit IsU2RxBusy;
#endif

//模拟UART3接收数据缓冲区数组长度定义
#ifdef DEBUG_UART_RX
#define U3RxBuff_MAXSIZE   2
extern unsigned char U3RxBuffer[U3RxBuff_MAXSIZE];
extern unsigned char U3RxPtr;
extern bit IsU3RxBusy;
#endif

#define U3TxBuff_MAXSIZE   150
extern unsigned char U3TxBuffer[U3TxBuff_MAXSIZE];
extern unsigned char U3TxPtr;
extern unsigned char U3LdPtr;
extern unsigned char U3TxBitCount;
extern unsigned char U3TxRate;
extern bit IsU3TxBusy;



extern bit g_1ms_flag;
extern bit g_2ms_flag;
extern bit g_10ms_flag;
extern bit g_100ms_flag;
extern bit g_1s_flag;
extern unsigned long g_1s_times;
//extern bit g_1min_flag;
extern unsigned long idata g_sys_time_ms;

extern unsigned char idata FGcount;

extern unsigned char idata speedBak;	//电机控制
extern bit gbMotorFGEn;
extern unsigned char idata gBMotorFGSet;
extern unsigned char idata speed_dang;
extern bit sys_mode;
extern bit run_mode;
extern bit gbMotorDelay;
extern bit IsSpeedChanged;
extern bit IsFanRunning; 

extern unsigned int pwmDLbak;

extern signed long beats;

extern unsigned int gBuzzerTimes_cnt;
extern unsigned int gBuzzerEdge_cnt;

extern KEY_INFO_Typedef key_info;
extern IIC_Operations_Typedef IIC_Operations;

//extern bit IsSysFault;
extern bit IsStepMotorBusy;

//ADC
extern unsigned int n_data;
extern bit ADCfinish;
//UV
extern bit IsUVOn;
extern bit IsUVfault;
extern unsigned int uv_check_timeinterval;
extern bit IsUVCheck;
extern bit uv_check_flag;
extern char uv_check_times;

//负氧离子工作状态位
extern bit Is_ION_On;

//仓门打开标志，次标志是扫码时下发的打开仓门标志
extern bit Is_Door_Open;

//打开仓门的时间
extern unsigned long door_open_time;

//传感器数据
extern unsigned int PM25_value;
extern unsigned int PM25_value_bak;
//extern float PM1_value;
//extern float PM10_value;
//extern float HCHO_value;


//计费功能
extern Charge_Typedef charge_info;




//extern bit Is_selfcheck;

extern unsigned long sys_start_time;
extern unsigned long sys_stop_time;

//数码管显示的PM数据
extern unsigned int display_pm_value;



//M26 busy标志位
//extern bit IsM26busy;
//extern struct M26_CMD_Typedef m26_cmd_info;


//M26 IMEI号   字符串形式保存
//extern char M26_IMEI[20];
//因RAM空间限制，CCID和CHECK_ID公用同一个数组
extern char ccid[40];
extern bit Is_Get_CCID;
extern char device_id[40];
extern bit Is_Get_IMEI;
//设备是否激活标志位
extern bit Is_device_activate;
extern bit Is_m26_wakeup;

//读取信号质量时，检测误码率是否是0
extern bit Is_signal_err_code_zero;


//m26启动后和服务器通讯的初始化工作是否结束
extern bit is_m26_fogcloud_init;

//extern struct M26_register_net;




extern unsigned long nowtime_ms;
extern unsigned long nowtime_s;


extern bit dev_status;
//extern bit Is_status_sync;
extern bit sync_this_loop;
//如果同步数据上传错误或者超时，则下一次sync还需要上传设备状态
extern bit resync;


//收到筹码标志位
extern bit charge_confirm;
//收到DR命令后，需返回DR，收到DR标志位
extern bit dr_confirm;
extern bit Is_send_dr_confirm;

extern char order_num[41];

//用于计算上传数据的时间
extern unsigned long next_upload_data_time;

//wifi指示灯状态
extern bit wifi_led_state;


//是否被手动关闭标志位，包括手动关闭仓门和手动通过按键关闭
extern bit Is_close_by_man;


//下次执行同步事件的时间
//extern unsigned long next_sync_mstime;

//下次执行同步事件的时间
extern unsigned long next_sync_stime;

//快速或者慢速同步状态标志位，当收到开仓门指令后同步时间间隔变为5秒，持续3分钟
extern bit is_fast_sync;

//快速同步的开始时间
extern unsigned long fast_sync_start_time;


//检测2G卡信号标志位
extern bit Is_signal_check;
//检测信号时，和服务器通讯失败的次数
extern unsigned char signal_check_err_times;


//wifi控制MCU系统自检标志位
extern bit is_sys_auto_check;
extern bit is_auto_check_complete;

//wifi控制MCU系统自检标志位
extern bit is_sys_manual_check;

extern sys_check_t sys_check_info;


//上传自动测试结果标志位
extern bit is_upload_auto_check_result;

//上传手动测试结果标志位
extern bit is_upload_manual_check_result;




#endif
