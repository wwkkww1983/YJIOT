#include <stdio.h>
#include "global.h"
#include "sys_run.h"
#include "dc_motor.h"
#include "step_motor.h"
#include "sensor.h"
#include "buzzer.h"
#include "UV.h"
#include "wifi_uart.h"
#include "common.h"
#include "ION.h"
#include "debug_uart.h"
#include "TM1620.h"
#include "timer.h"
#include "user_timer.h"

//当仓门关闭时，仓门检测引脚是高电平；当仓门打开时，仓门检测引脚是低电平


void sys_data_clear(void)
{

}

//防止仓门坏了，开机时必须进行仓门检测。如果仓门坏了再打开风机可能会烧坏风机
void sys_init_check(void)
{
    unsigned char delay_1s_times = 0;
    
//    if(sys_mode == STANDBY && IsStepMotorBusy == 0)
//    {
//        if(!DOOR_CHECK_PIN)
//        {
//            IsSysFault = 1;
//        }
//    }  
  
    if(!DOOR_CHECK_PIN)
    {
        Door_Close();
    }
    else
    {
        return;
    }
    while(delay_1s_times < 10)
    {
        if(_test_timeflag(g_2ms_flag))
        {
            g_2ms_flag = 0;
            TurnMotor();           
        } 
      
        if(_test_timeflag(g_1s_flag))
        {
            g_1s_flag = 0;
            delay_1s_times += 1;
            if(beats == 0)
            {
                //IsSysFault = 0;
                return;
            }           
        }
    }
}



void sys_start(void)
{
    sys_start_time = get_sys_stime();
    //PM25_value_bak = PM25_value;
    UV_On();
    ION_On();
    Buzzer_Power_On();  
  
    //开启UV灯后延时一段时间，不让所有设备同时启动  
    while(get_sys_stime() < (sys_start_time + 1)) ;

    if(Is_Door_Open == 0 || (Is_Door_Open == 1 && DOOR_CHECK_PIN == 1))
    {
      Door_Open();
    }
    Start_DC_Motor();
    
    
    sys_mode = RUNNING;
    IsFanRunning = 1;
    IsSpeedChanged = 1;
    TM1620_LED_Control(LED_SLEEP_MODE,LED_OFF);
    TM1620_LED_Control(LED_SPEED_LOW,LED_ON);
}

void sys_stop(void)
{
    sys_stop_time = get_sys_stime();
    Buzzer_Power_Off();
    sys_mode = STANDBY;
    Stop_DC_Motor();
    Door_Close();
    UV_Off();
    ION_Off();
    sys_data_clear();   
    IsSpeedChanged = 1;
    Is_Door_Open = 0;
    //run_mode = 0;
    TM1620_LED_Control(LED_ALL,LED_OFF);
    TM1620_LED_Control(LED_SLEEP_MODE,LED_ON);
  
    IsUVfault = 0;
    
    if(user_timer_info.timer_state == 1)
    {
        stop_user_timer();
    }
    
  
    if(run_mode == 1)
    {  
       stop_sys_smart_mode();
    }
    
}


void sys_run(void)
{
//    unsigned char wifi_count_times = 0;
//    unsigned long nowtimes = 0;
    //unsigned char debug_buff[30] = {0};
    
    //wifi配网永远有效,wifi配网需组合按键持续按下超过2秒才进入配网模式，防止误操作
    if(key_info.IsTouchedKey == 1 && key_info.WhichKey == KEY_WIFI)
    {
        wifi_earse_easylink_cmd();
        Clear_Touch_Info();
    }
    
    //如果有人把仓门关闭了，则机器进入待机状态
    check_if_doorclose_manual();
    
  
    if(key_info.IsTouchedKey == 1)
    {
        switch(key_info.WhichKey)
        {
          case KEY_POWER:
            if(is_sys_manual_check == 1)
            {
                sys_check_info.touch_key_check |= (1 << power_bit);
            }  
          
            if(sys_mode == STANDBY)
            {
                sys_start();             
            }
            else if(sys_mode == RUNNING)
            { 
                sys_stop();              
            }
            break;
          case KEY_SPEED:
            if(sys_mode == RUNNING)
            {
                Buzzer_Speed();
                if(speed_dang < HIGH_SPEED )
                {
                    speed_dang++;
                    IsSpeedChanged = 1;
                }
                else if(speed_dang == HIGH_SPEED)
                {
                    speed_dang = QUIET_SPEED;
                    IsSpeedChanged = 1;
                }
                else
                {
                    break;
                }
                TM1620_LED_Control(speed_dang + 1,LED_ON);
                
                if(is_sys_manual_check == 1)
                {
                    if(speed_dang == QUIET_SPEED)
                    {
                        sys_check_info.touch_key_check |= (1 << quiet_speed_bit);
                    }
                    else if(speed_dang == LOW_SPEED)
                    {
                        sys_check_info.touch_key_check |= (1 << low_speed_bit);
                    }
                    else if(speed_dang == MID_SPEED)
                    {
                        sys_check_info.touch_key_check |= (1 << mid_speed_bit);
                    }
                    else if(speed_dang == HIGH_SPEED)
                    {
                        sys_check_info.touch_key_check |= (1 << high_speed_bit);
                    }
                } 
                
                //在自动模式下，如果手动调节风量了，则取消自动模式
                if(run_mode == 1)
                {
                    stop_sys_smart_mode();
                }
                
            }
            else
            {
                break;
            }
            break;
          case KEY_TIMER:
            if(is_sys_manual_check == 1)
            {
                sys_check_info.touch_key_check |= (1 << timer_bit);
            } 
            
            if(sys_mode == RUNNING)
            {
                if(user_timer_type < USER_TIMER_4H)
                {
                    user_timer_type += 1;
                    set_user_timer(user_timer_type);
                }
                else
                {
                    stop_user_timer();
                }
                Buzzer_Touch_Key();               
            }           
            
            break;
          case KEY_MODE:
            if(is_sys_manual_check == 1)
            {
                sys_check_info.touch_key_check |= (1 << mode_bit);
            }  
            
            if(sys_mode == RUNNING)
            {
                run_mode = !run_mode;
                if(run_mode == 0)
                {
                    stop_sys_smart_mode();
                }
                else
                {
                    set_sys_to_smart_mode();
                }
                Buzzer_Touch_Key();     
            }          
            break;
          case KEY_WIFI:
            //control wifi to easylink mode  
            if(is_sys_manual_check == 1)
            {
                sys_check_info.touch_key_check |= (1 << wifi_bit);
            }            
            break;            
            
            
          default:
            break;
            
        
        }
        if(key_info.IsTouchedKey == 1 && key_info.WhichKey != KEY_WIFI)
        {
            Clear_Touch_Info();
        }
        
    }
    
    if(run_mode == 1)
    {
        sys_smart_mode();
    }

}





#define GET_SENSOR_TIME_INTERVAL    (2)
#define RUN_CONTINUE_TIME    (1*60)
#define STOP_CONTINUE_TIME   (10*60)   //10min
#define STANDBY_UPDATA_DATA_INTERVAL  (15*60)   //15min
//#define RUN_CONTINUE_TIME    (1*60)
//#define STOP_CONTINUE_TIME   (1*60)   //10min
//#define STANDBY_UPDATA_DATA_INTERVAL  (1*60)   //15min

void display_pm_data(void)
{
    static bit change_to_3min_updata = 0;
    static bit IsPowerOn = 0;
    static unsigned long get_pm_trigger_time = 0;
    static unsigned long standby_trigger_time = 0;
    static unsigned int temp_pm_value = 0;
    unsigned long nowtime = 0;
 
    //unsigned char pm_adjust = 0;
    //const unsigned int get_sensor_data_time_interval = 5 * 1000;
  
//    unsigned char debug_buff[30] = {0};
    
    //unsigned long temp = 0;
//    unsigned long temp_low,temp_high = 0;
//    unsigned long temp_1 = 0;
  
  
    //get_sensor_data_time_interval = GET_SENSOR_TIME_INTERVAL;
  
    nowtime = get_sys_stime();
    
    if(nowtime <= GET_SENSOR_TIME_INTERVAL && get_pm_trigger_time >= (0xFFFFFFFF - GET_SENSOR_TIME_INTERVAL))
    {
        get_pm_trigger_time = nowtime + GET_SENSOR_TIME_INTERVAL;
    }
    
    // 周期性获取传感器数据，时间间隔 GET_SENSOR_TIME_INTERVAL ，单位s
    if(nowtime >= get_pm_trigger_time)
    {
        PM25_value = Read_PMSensor_Data();
        //get_pm_trigger_time = nowtime + get_sensor_data_time_interval;
        get_pm_trigger_time = nowtime + (GET_SENSOR_TIME_INTERVAL | 0x00);
        
      
        //5秒钟更新一次数码管显示的数据
        if(is_sys_auto_check == 0)
        {
            TM1620_DispalyData(SENSOR_PM25,display_pm_value);
            //TM1620_DispalyData(SENSOR_PM25,PM25_value);
        }
        

    }
    if(sys_mode == RUNNING)
    {
        if(IsPowerOn == 0)
        {
            IsPowerOn = 1;
        }
        if(change_to_3min_updata == 1)
        {
            change_to_3min_updata = 0;
        }
        
        if(nowtime <= (sys_start_time + (RUN_CONTINUE_TIME | 0x00)))
        {
            //打开设备后的 60 秒内显示开机前的数值
            if(nowtime <= 180)
            {
                //设备刚上电，此时显示实时数值
                if(PM25_value_bak < PM25_value)
                {
                    display_pm_value = (unsigned int)PM25_value;
                    PM25_value_bak = (unsigned int)PM25_value;
                }
                else
                {
                    display_pm_value = PM25_value_bak;
                }

              
//               mymemset(debug_buff,0,30);
//               sprintf(debug_buff,"step1\n");
//               DEBUG_Uart_Sendbytes(debug_buff,mystrlen(debug_buff));
            }
            else
            {
                //此种情况是设备已经上电一段时间，从待机进入运行模式，为了防止显示的数值上涨，显示待机时的数值
                display_pm_value = (unsigned int)PM25_value_bak;
              
              
//               mymemset(debug_buff,0,30);
//               sprintf(debug_buff,"step2\n");
//               DEBUG_Uart_Sendbytes(debug_buff,mystrlen(debug_buff));              
              
            }

        }
        else
        {
            //取两次获取到的数据的平均值，防止数据跳变过快
            display_pm_value = ((unsigned int)PM25_value + PM25_value_bak) / 2;
            PM25_value_bak = (unsigned int)PM25_value;
        }

    }
    else if(sys_mode == STANDBY)
    {
        
        //系统待机后，正常情况下PM2.5数据会下降，此处数据进行处理，待机后10分钟内如果数据下降则显示刚待机时的数据
        if(IsPowerOn == 0)
        {
            //刚开机前3分钟内显示实时数据
            if(nowtime < (STANDBY_UPDATA_DATA_INTERVAL | 0x00))
            {
                if(PM25_value_bak < (unsigned int)PM25_value)
                {
                    PM25_value_bak = (unsigned int)PM25_value;
                }
                display_pm_value = PM25_value_bak; 
                goto exit;              
            }
            else
            {
                IsPowerOn = 1;
                change_to_3min_updata = 1;
                temp_pm_value = 0;
                standby_trigger_time = nowtime + (STANDBY_UPDATA_DATA_INTERVAL | 0x00);
            }
        }
        else
        {
            if(change_to_3min_updata == 0)
            {
                if(nowtime < (sys_stop_time + (STOP_CONTINUE_TIME | 0x00)))
                {
                    //从运行模式进入待机后的10min内，PM的数据不能小于刚进入待机模式时的数据
                    if(PM25_value < PM25_value_bak)
                    {
                        //取平均值是为了防止显示数据变化大
                        display_pm_value = (PM25_value_bak + display_pm_value) / 2;
                    }
                    else
                    {
                        //防止刚进入待机时数据变化范围太大
                        display_pm_value = ((unsigned int)PM25_value + display_pm_value + PM25_value_bak) / 3;
                    }                        
                } 
                else
                {
                    change_to_3min_updata = 1; 
                    temp_pm_value = 0;
                    standby_trigger_time = nowtime + (STANDBY_UPDATA_DATA_INTERVAL | 0x00);
                }                  
            }
            else
            {
                //获取 STANDBY_UPDATA_DATA_INTERVAL  分钟内最大的数据，用于显示
                if(temp_pm_value <= (unsigned int)PM25_value)
                {
                    temp_pm_value = (unsigned int)PM25_value;

                }
                
                if(PM25_value_bak < (unsigned int)PM25_value)
                {
                    PM25_value_bak = (unsigned int)PM25_value;
                    display_pm_value = ((unsigned int)PM25_value + display_pm_value) / 2;
                        
                }               
            
                if(nowtime >= standby_trigger_time)
                {
                    //上次显示的数据和这次获取到的最大值求和，然后取平均值，不然可能会导致2次显示的数据跳变比较大
                    display_pm_value = (temp_pm_value + PM25_value_bak) / 2;
                    PM25_value_bak = display_pm_value;
                    temp_pm_value = 0;
                     
                    standby_trigger_time += (STANDBY_UPDATA_DATA_INTERVAL | 0x00);
                  
                  
//                    temp_low = standby_trigger_time & 0xFFFF;     
//                    temp_high = (standby_trigger_time >> 16) & 0xFFFF;   
//                    mymemset(debug_buff,0,30);
//                    sprintf(debug_buff,"trigger time:0x%x%4x\n",(unsigned int)temp_high,(unsigned int)temp_low);
//                    DEBUG_Uart_Sendbytes(debug_buff,mystrlen(debug_buff));  
                } 
              
            }
      
        }
        
    }
    
 exit:
    if(display_pm_value < 2)
    {
        display_pm_value = 2;
    }
    
    return;
    
    
}



//在自动模式下，风机档位切换后10秒钟内不允许再次切换，否则PM在临界值时会非常频繁的切换档位，体验不好
void sys_smart_mode(void)
{
    
    static bit speed_changed = 0;   //防止PM浓度在临界值时，频繁切换档位
    static bit IsLedChange = 0;
    static unsigned long speed_changed_time = 0;   //上次切换风挡时的时间
  
    const unsigned char speed_continue_time = 10;  //10秒钟内不能再次切换风挡
  
    unsigned long nowtime = 0;
  
    
  
    if(sys_mode == STANDBY)
    {
        return;
    }
    
    nowtime = get_sys_stime();
    
    if(speed_changed == 1)
    {
        //风机档位刚切换过，10秒以后才能切换
        if(nowtime <= (speed_changed_time + speed_continue_time))
        {
            return;
        }
        else
        {
            speed_changed = 0;
        }
        
    }
    
    if(display_pm_value < PM25_QUIET_SPEED )
    {
        if(speed_dang != QUIET_SPEED)
        {
            speed_dang = QUIET_SPEED;
            IsSpeedChanged = 1;
            speed_changed = 1;
            IsLedChange = 1;
        }       
    }
    else if(display_pm_value < PM25_LOW_SPEED)
    {
        if(speed_dang != LOW_SPEED)
        {
            speed_dang = LOW_SPEED;
            IsSpeedChanged = 1;
            speed_changed = 1;
            IsLedChange = 1;
        }  
    }
    else
    {
        if(speed_dang != HIGH_SPEED)
        {
            speed_dang = HIGH_SPEED;
            IsSpeedChanged = 1;
            speed_changed = 1;
            IsLedChange = 1;
        }
    }   

    if(IsLedChange == 1)
    {
        //自动模式下，风挡变后，LED灯也要相应的变化
        TM1620_LED_Control(speed_dang + 1,LED_ON);
        IsLedChange = 0;       
    }      

}





//系统运行时，如果有人手动把仓门关闭了，则机器进入待机模式
//检测方法：100ms检测一次仓门，如果连续2秒钟仓门都是关闭的，则机器进入待机模式
#define MAX_DOOR_CLOSE_TIMES    20
void check_if_doorclose_manual(void)
{
    static unsigned char check_doorclose_times = 0;
    unsigned long nowtime = 0;
    
  
    if(sys_mode == RUNNING)
    {
        nowtime = get_sys_stime();
        if(nowtime >= (sys_start_time + 10))
        {
            if(g_100ms_flag == 1)
            {
                if(DOOR_CHECK_PIN == 1)
                {
                    check_doorclose_times += 1;
                }
                else
                {
                    check_doorclose_times = 0;
                }
                
                if(check_doorclose_times >= MAX_DOOR_CLOSE_TIMES)
                {
                    if(sys_mode == RUNNING)
                    {
                        sys_stop();
                        check_doorclose_times = 0;
                    }
                }
            }
        }
    }

}


void set_sys_to_smart_mode(void)
{
    run_mode = 1;
    TM1620_LED_Control(LED_AUTO_MODE,LED_ON);
}

void stop_sys_smart_mode(void)
{
    run_mode = 0;
    TM1620_LED_Control(LED_AUTO_MODE,LED_OFF);
}


//系统自检
//当仓门关闭时，仓门检测引脚是高电平；当仓门打开时，仓门检测引脚是低电平
//status是系统检测标志，bit0表示仓门，bit1表示风机，bit2表示uv灯

//sys_check_info.status的bit位为1表示检测设备有问题，0表示正常

//此函数的执行间隔必须是100ms，因为FGcount的采集周期是100ms
void sys_dev_auto_check(void)
{

    //系统检测UV和风机的时间
    static const unsigned long sys_check_time = 30;
    unsigned long nowtime = 0;
  
    sys_check_info.fg_count = FGcount;
    FGcount = 0;  

  
    //step0  关机
    if(sys_check_info.step == 0)
    {
        if(sys_mode == RUNNING)
        {
            sys_stop();
            is_sys_auto_check = 1;
        }
        sys_check_info.step += 1;
        is_auto_check_complete = 0;
    }     
    else if(sys_check_info.step == 1)  //step1  检测仓门是否可以关闭或者检测开关是否有问题
    {        
        if(beats == 0)
        {
            if(DOOR_CHECK_PIN == 0)
            {
                //仓门故障
                //sys_check_info.status = ((1 << door_bit) | (1 << fan_bit) | (1 << uv_bit));
                is_auto_check_complete = 1;
                goto exit;
            }
            else
            {               
                sys_check_info.step += 1;
            }
        }        
    }
    else if(sys_check_info.step == 2)    //step2  打开机器
    {       
        sys_start();
        sys_check_info.step += 1;             
    }
    else if(sys_check_info.step == 3)    //step3 检测仓门是否打开
    {
        if(beats == 0)
        {
            if(DOOR_CHECK_PIN == 0)
            {
                //仓门正常,对应的bit位清零
                sys_check_info.status &= ~(1 << door_bit);
                //sys_check_info.status = 0;
                sys_check_info.step += 1;
            }
            else
            {  
                //sys_check_info.status = ((1 << door_bit) | (1 << fan_bit) | (1 << uv_bit));
                is_auto_check_complete = 1;
                goto exit;
            }
        }
          
        
    }
    else if(sys_check_info.step == 4)    //step4  检测风机和UV灯
    {
        nowtime = get_sys_stime();
        //检测风机和UV灯有效时间20秒
        if(nowtime >= (sys_start_time + 10) && nowtime <= (sys_start_time + sys_check_time))
        {
//            if(IsUVfault == 1)
//            {
//                sys_check_info.status |= (1 << uv_bit);
//            }
            

            if(sys_check_info.fg_count < (SPEED_FG_COUNT2 - 1) || sys_check_info.fg_count > (SPEED_FG_COUNT2 + 1))
            //if(sys_check_info.fg_count < 12 || sys_check_info.fg_count > 14)
            {              
                if(sys_check_info.fan_check_fault_times < 0xFF)
                {
                    sys_check_info.fan_check_fault_times += 1;
                }
            }
                         
                        
        }
        else if(nowtime > (sys_start_time + sys_check_time))
        {
            //在Dcmoto_adj()函数中判断FGcount反馈信号，100ms检测一次，20秒共检测200次，反馈信号并不是一个固定值，而是在一定范围内波动，按出错概率进行检测
          
            if(IsUVfault == 0)
            {
                sys_check_info.status &= ~(1 << uv_bit);
            }
            
            if(sys_check_info.fan_check_fault_times < 30)
            {
                sys_check_info.status &= ~(1 << fan_bit);
            }
            if((int)PM25_value > 0)
            {
                sys_check_info.status &= ~(1 << pm25_bit);
            }
            is_auto_check_complete = 1;
        }
    }

    
    
    
    
exit:
    if(is_auto_check_complete == 1)
    {
        stop_sys_auto_check();        
    }
}


void start_sys_auto_check(void)
{
    is_sys_auto_check = 1;
    sys_check_info.step = 0;
    sys_check_info.status = 0xFF;
    sys_check_info.fan_check_fault_times = 0;  
    
}

void stop_sys_auto_check(void)
{
    is_sys_auto_check = 0;
    is_auto_check_complete = 0;
    sys_check_auto_result_upload();
    if(sys_mode == RUNNING)
    {
        sys_stop();
    }
}

void start_sys_manual_check(void)
{
    is_sys_manual_check = 1;
    sys_check_info.touch_key_check = 0;
    sys_check_info.start_time = get_sys_stime();
    Buzzer_Get_Charge();
    
}

void check_if_stop_manual_check(void)
{
    //手动检测的最大时间，如果3分钟内不停止则自动停止
    const unsigned long check_max_time = 180;
    if(get_sys_stime() >= (sys_check_info.start_time + check_max_time))
    {        
        stop_sys_manual_check();
    }
}


void stop_sys_manual_check(void)
{
    is_sys_manual_check = 0;
    sys_check_manual_result_upload();
    Buzzer_Get_Charge();
}



