#ifndef __DC_MOTOR_H__
#define __DC_MOTOR_H__

#include "PWM.h"

//定义的风机的转速单位是r/min
#define FAN_SPEED1       380    //睡眠档
#define FAN_SPEED2       620    //低档
#define FAN_SPEED3       760    //中档
//#define FAN_SPEED4       900    //高档
#define FAN_SPEED4       950    //高档

//风机每转一圈FG反馈12个脉冲
//风机的反馈信号FG每100ms是一个计数周期，下面定义的FG是100ms的计数
//风机转速和100ms FG的对应关系是 (FAN_SPEED/60)*12/10 = (FAN_SPEED*12)/(60*10) = (FAN_SPEED*2)/100 = FAN_SPEED/50
#define SPEED_FG_COUNT1		(8)//(FAN_SPEED1/50) 
#define SPEED_FG_COUNT2		(13)//(FAN_SPEED2/50) 
#define SPEED_FG_COUNT3		(16)//(FAN_SPEED3/50) 
//#define SPEED_FG_COUNT4		(18)//(FAN_SPEED4/50)
#define SPEED_FG_COUNT4		(19)//(FAN_SPEED4/50)
/*
净化器实测档位与模拟量的关系：
静音：  2.66V
低档：  3.35V
中档:   3.70V
高档：  4.36V

占空比对应的VSP的输出电压实际测试情况
560 2.75V
580 2.78V
650 3.21V
670 3.30V
675 3.34V
740 3.67V
745 3.69V
750 3.73V
870 4.33V
875 4.37V
890 4.43V
*/

//wifi board
//PWMDL_SPEED都定义为100的整数倍
//#define PWMDL_START			6000 
//#define PWMDL_SPEED1		9000  
//#define PWMDL_SPEED2		10500 
//#define PWMDL_SPEED3		11600 
//#define PWMDL_SPEED4		12500
//#define PWMDL_MAX		    13500

//4.36V
//#define PWMDL_START			6000 
//#define PWMDL_SPEED1		9000  
//#define PWMDL_SPEED2		11000 
//#define PWMDL_SPEED3		12100 
//#define PWMDL_SPEED4		14200
//#define PWMDL_MAX		    14500

//m26 board
#define PWMDL_START			375 
#define PWMDL_SPEED1		580 
#define PWMDL_SPEED2		685 
#define PWMDL_SPEED3		745
#define PWMDL_SPEED4		875
#define PWMDL_MAX		    910


#define DC_MOTOR_PWM  PWM1_NUM


enum DC_MOTOR_SPEED{
    
    DC_MOTOR_SPEED0   = 0,
    QUIET_SPEED       = 1,
    LOW_SPEED         = 2,
    MID_SPEED         = 3,
    HIGH_SPEED        = 4
    

};




void Start_DC_Motor(void);
void Stop_DC_Motor(void);

void Set_DC_Motor_Speed(unsigned char speed);

void DirectMotor(void);
void Dcmoto_adj(void);




#endif