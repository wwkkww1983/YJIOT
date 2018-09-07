#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "M26.h"
#include "wifi_uart.h"
#include "debug_uart.h"
#include "global.h"
#include "timer.h"
#include "TM1620.h"
#include "dc_motor.h"
#include "UV.h"
#include "ION.h"
#include "sys_run.h"
#include "step_motor.h"
#include "buzzer.h"

//AT指令发送时间
M26_Cmd_Typedef m26_cmd_info;

M26_RESGISTER_Typedef M26_register_net;

//M26_DEV_INFO_Typedef m26_dev_info;


//m26与服务器http通讯第几步标识
static bit Is_http = 0;
static unsigned char http_step = 0;
//m26动作标识字节，bit0表示激活，bit1表示版本更新，bit2表示数据上报，bit3表示状态更新，bit4表示筹码确认
unsigned char m26_action = 0;

//电源芯片控制管脚低电平时，电源芯片工作
void M26_Power_Pin_Init(void)
{
    M26_POWER_PIN_PxM0 |= (1 << M26_POWER_PIN_PORTBIT);
    M26_POWER_PIN_PxM1 &= ~(1 << M26_POWER_PIN_PORTBIT);
  
    M26_POWER_PIN = 1;
}

void M26_Power_On(void)
{
    M26_POWER_PIN = 0;   
}

void M26_Power_Off(void)
{
    M26_POWER_PIN = 1;
    //重启M26的话断电保持一段时间，防止启动过快
    if(g_1s_flag == 1)
    {
        g_1s_flag = 0;
    } 
    while(g_1s_flag == 0) ;
    
    g_1s_flag = 0;
    while(g_1s_flag == 0) ;
}

//PWK管脚高电平保持3秒后M26开机
void M26_Wakeup_Pin_Init(void)
{
    M26_WAKEPU_PIN_PxM0 |= (1 << M26_WAKEPU_PIN_PORTBIT);
    M26_WAKEPU_PIN_PxM1 &= ~(1 << M26_WAKEPU_PIN_PORTBIT);
  
    M26_WAKEPU_PIN = 0;
}

void M26_Wakeup(void)
{
    unsigned char times_s = 0;
    M26_WAKEPU_PIN = 1;
  
    if(g_1s_flag == 1)
    {
        g_1s_flag = 0;
    } 
    //wakeup管脚拉高持续3秒

    while(times_s <= 6)
    {
        if(g_1s_flag == 1)
        {
            g_1s_flag = 0;
            times_s += 1;
        }
    }
  
    M26_WAKEPU_PIN = 0;

}

void M26_Restart(void)
{
    //重启m26       
    M26_Power_Off();
    M26_Power_On();
      
    //刚上电延时一点时间
    if(g_1s_flag == 1)
    {
        g_1s_flag = 0;
    }
    while(g_1s_flag == 0);
        
    M26_Wakeup();
        
    M26_register_net.step = 0;
    http_step = 0;
    Is_http = 0;
    m26_action = 0;
    
    Is_Get_CCID = 0;
    Is_signal_err_code_zero = 0;
    Is_device_activate = 0;
    M26_register_net.Is_m26_registerd = 0;
        
    m26_cmd_info.error_times = 0;
    m26_cmd_info.timeout_times = 0;
    
    is_m26_fogcloud_init = 0;
    
    is_upload_auto_check_result = 0;
    
    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
    sprintf(m26_cmd_info.sendtring,"m26 restart\r\n");
    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,strlen(m26_cmd_info.sendtring));
    
}

unsigned char make_m26_send_string(char *str_buff,char *cmd,char *cmd_para)
{
    if(str_buff == NULL)
    {
        return 1;
    }
    
    if(cmd == NULL && cmd_para == NULL)
    {
        return 2;
    }
    
    if(cmd != NULL)
    {
        if(str_buff[0] != 0)
        {
            //strcat(str_buff,cmd);
            
        }
        else
        {
            strcpy(str_buff,cmd);
        }
          
        
    }
    
    //有几个指令带参数，先添加参数再加\r\n
    if(cmd_para != NULL)
    {
        strcat(str_buff,cmd_para);
    }
    
    strcat(str_buff,M26_CMD_END_FLAG);
    
    return 0;
}


int M26_wait_response(void)
{
    int res = M26_WAIT_RESPONSE;
    //unsigned char i,j = 0;
    //unsigned char num = 0;
    char *cmd_position = NULL;
    //unsigned char debug_buff[180] = {0};
    
  
    //如果之前没有发送过数据或者命令，则直接返回
    if(m26_cmd_info.Is_wait_data == 0)
    {
        res = CMD_IDLE;
        m26_cmd_info.cmd_result = CMD_IDLE;
        goto exit;
    }
    
    nowtime_ms = get_sys_mstime();
    
    
    //防止时间溢出
    if(m26_cmd_info.cmd_send_time > 0xF0000000 && nowtime_ms < 0xE0000000)
    {
        m26_cmd_info.cmd_send_time = nowtime_ms;
    }

    
    //超时 没有返回数据
    if(nowtime_ms >= (m26_cmd_info.cmd_send_time + (M26_TIME_OUT | 0x00)))
    {
        m26_cmd_info.cmd_result = CMD_TIMEOUT;
        
        res = CMD_TIMEOUT;
        goto cmd_failed;
    }
    
    if(strstr(U0RxBuffer,END_ERROR) != NULL)
    {
        m26_cmd_info.cmd_result = CMD_ERROR;
        res = CMD_ERROR;
        goto cmd_failed;
    }
    
    if(m26_cmd_info.type == STRING_TYPE)
    {
        //发送字符串时返回OK或者ERROR
        if(strstr(U0RxBuffer,m26_cmd_info.end_flag) != NULL)
        {
            m26_cmd_info.cmd_result = CMD_SUCESS;   
            res = CMD_SUCESS; 
            goto  cmd_sucess;       
        }
      
    }
    else if(m26_cmd_info.type == CMD_TYPE)
    {
        
        //发送命令时会返回命令+OK或者命令+ERROR
        cmd_position = strstr(U0RxBuffer,m26_cmd_info.cmd);
        if(cmd_position != NULL && strstr(cmd_position,m26_cmd_info.end_flag) != NULL)
        //if(strstr(U0RxBuffer,"OK") != NULL)
        {            
            res = CMD_SUCESS;
            m26_cmd_info.cmd_result = CMD_SUCESS;   
        
            goto cmd_sucess;
        }

        
    }
    

exit:    
    return res;
    
cmd_sucess:
    //m26_cmd_info.cmd_result = CMD_SUCESS;
    return res;
    
    
cmd_failed:
    //m26_cmd_info.cmd_result = res;
    return res;
    
}


void check_m26_cmd_result(void)
{    
    char *position = NULL;
    int value_int = -1;
    long value_long = -1;
    int status = -1;
    unsigned char ordnum_cmp_result = 0;
  
    int check_type = -1;
    int check_state = -1;
    
  
    unsigned char i = 0;
    
    //char debug_buff[40] = {0};
    
    if(m26_cmd_info.Is_wait_data == 0)
    {
        return;
    }
    else if(m26_cmd_info.Is_wait_data == 1)
    {
        if(m26_cmd_info.cmd_result == CMD_WAIT)
        {
            return;
        }
        else if(m26_cmd_info.cmd_result == CMD_SUCESS)
        {

          
            m26_cmd_info.Is_wait_data = 0;
          
            //http发送url后再发送字符串并不返回命令，只返回OK
            if(Is_http == 1 && (http_step == 1 || http_step == 3))
            {
                http_step += 1;
                return;
            }

          
            if(strcmp(m26_cmd_info.cmd,M26_ATI_CMD) == 0)
            {
            
            }
            else if(strcmp(m26_cmd_info.cmd,M26_CSQ_CMD) == 0)
            {
                //获取信号质量
                //debug
                DEBUG_Uart_Sendbytes(U0RxBuffer,strlen(U0RxBuffer)); 
              
                position = strstr(U0RxBuffer,",");
                if(position != NULL)
                {
                    for(i = 0;i < strlen(U0RxBuffer);i++)
                    {
                        if(*(position + 1 + i) == ASCII_RETURN || *(position + 1 + i) == ASCII_NEW_LINE)
                        {
                            if(i != 0)
                            {
                                mymemset(m26_cmd_info.cmd_para,0,sizeof(m26_cmd_info.cmd_para));
                                strncpy(m26_cmd_info.cmd_para,(position + 1),i); 
                                value_int = atol(m26_cmd_info.cmd_para);
                              
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"i = %d,signal err code:%d\r\n",(unsigned int)i,(unsigned int)value_int);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                              
                                if(value_int == 0)
                                {
                                    Is_signal_err_code_zero = 1;
                                }
                                else
                                {
                                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                    sprintf(m26_cmd_info.sendtring,"signal err code not equal zero,wait\r\n");
                                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                                }
                            }
                            else
                            {
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"not find signal err code\r\n");
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                            }                           
                            break;
                        }                      
                    }
                }               
            }
            else if(strcmp(m26_cmd_info.cmd,M26_GSN_CMD) == 0)
            {
                //获取IMEI成功
//                mymemset(M26_IMEI,0,sizeof(M26_IMEI));
//                position = strstr(U0RxBuffer,M26_CMD_END_FLAG);
//                strncpy(M26_IMEI,position + 2,M26_IMEI_LEN);
              
//                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                sprintf(m26_cmd_info.sendtring,"IMEI:%s\n",M26_IMEI);
//                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                //Is_Get_IMEI = 1;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QCCID_CMD) == 0)
            {
                //获取CCID成功
                mymemset(ccid,0,sizeof(ccid));
                position = strstr(U0RxBuffer,M26_CMD_END_FLAG);
                strncpy(ccid,position + 2,M26_CCID_LEN);
              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"CCID:%s\n",ccid);
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                Is_Get_CCID = 1;
            }            
            else if(strcmp(m26_cmd_info.cmd,M26_QSPN_CMD) == 0)
            {     
                //读取SIM 卡服务运营商名称             
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QSPN_CMD OK\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 1;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QIFGCNT_CMD) == 0)
            {     
                //设置前景模式              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QIFGCNT_CMD OK\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 2;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QICSGP_CM_CMD) == 0)
            {   
                //设置链接模式              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QICSGP_CM_CMD OK\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                //M26_register_net.Is_m26_registerd = 1;
                M26_register_net.step = 3;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QIREGAPP_CMD) == 0)
            {   
                //启动任务并设置接入点             
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QIREGAPP_CMD OK\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                //M26_register_net.Is_m26_registerd = 1;
                M26_register_net.step = 4;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QIACT_CMD) == 0)
            {   
                //激活网络            
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QIACT_CMD OK\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.Is_m26_registerd = 1;
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QHTTPURL_CMD) == 0)
            {   
                //set HTTP Server URL          
//                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                sprintf(m26_cmd_info.sendtring,"M26_QHTTPURL_CMD OK\n");
//                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              

                http_step += 1;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QHTTPPOST_CMD) == 0)
            {   
                //http post          
//                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                sprintf(m26_cmd_info.sendtring,"M26_QHTTPPOST_CMD OK\n");
//                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                http_step += 1;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QHTTPREAD_CMD) == 0)
            {
                m26_cmd_info.error_times = 0;
                m26_cmd_info.timeout_times = 0;              
                //http read          
//                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                sprintf(m26_cmd_info.sendtring,"M26_QHTTPREAD_CMD OK\r\n");
//                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
               
                if(M26_BIT_CHECK(m26_action,HTTP_ACTIVATE))
                {
                    //激活，获取到device_id
                    if((position = strstr(U0RxBuffer,"DID")) != NULL)
                    {
                        mymemset(device_id,0,sizeof(device_id));
                        strncpy(device_id,position + 6,DEVICE_ID_LEN);
                      
                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                        sprintf(m26_cmd_info.sendtring,"id:%s\n",(char *)device_id);
                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                    }
                    Is_device_activate = 1;
                }
                else if(M26_BIT_CHECK(m26_action,HTTP_OTA))
                {
                    //ota check
                  
                    is_m26_fogcloud_init = 1;

                    DEBUG_Uart_Sendbytes(U0RxBuffer,mystrlen(U0RxBuffer));
                  
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"ota check sucess\r\n");
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));                  
                }
                else if(M26_BIT_CHECK(m26_action,HTTP_UPLOAD_DATA))
                {                   
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"upload data sucess\r\n");
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));                
                }
                else if(M26_BIT_CHECK(m26_action,HTTP_SYNC))
                {
                  
//                    DEBUG_Uart_Sendbytes(U0RxBuffer,strlen(U0RxBuffer)); 
//                    goto test_label;
                    //设备状态同步
                    //设备状态同步发送成功后，resync清零
                    resync = 0;
                    if(dr_confirm == 1)
                    {
                        dr_confirm = 0;
                    }
                  
                    //debug
//                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                    sprintf(m26_cmd_info.sendtring,"%s\r\n",U0RxBuffer);  
//                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));     
                    //DEBUG_Uart_Sendbytes(U0RxBuffer,mystrlen(U0RxBuffer));                    
                    
                    if((position = strstr(U0RxBuffer,"SF")) == NULL)
                    {
                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                        sprintf(m26_cmd_info.sendtring,"not find SF\r\n");
                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));                   
                    }
                    else
                    {
                        value_int = -1;
                        value_int = string_get_int(position);
                      
//                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                        sprintf(m26_cmd_info.sendtring,"SF val:%d\r\n",(unsigned int)value_int);
//                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                        if(value_int == 1)
                        {                         
                            //获取到的字符串中存在控制信息
                            position = strstr(U0RxBuffer,"SM");
                            if(position != NULL)
                            {
                                //手自动模式的改变
                                value_int = -1;
                                value_int = string_get_int(position);
                              
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"SM:%d\r\n",(unsigned int)value_int);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));                              
                              
                                if(value_int == 1 && run_mode == 0)
                                {
                                    set_sys_to_smart_mode();
                                }  
                                else if(value_int == 0 && run_mode == 1)
                                {
                                    stop_sys_smart_mode();
                                }                                 
                            } 
                            position = strstr(U0RxBuffer,"FA");
                            if(position != NULL)
                            {
                                //风机控制
                                value_int = -1;
                                value_int = string_get_int(position);
                              
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"FA:%d\r\n",(unsigned int)value_int);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                              
                                Set_DC_Motor_Speed((unsigned char)value_int);
                              
                            }
                            position = strstr(U0RxBuffer,"UV");
                            if(position != NULL)
                            {
                                //UV灯控制
                                value_int = -1;
                                value_int = string_get_int(position);
                              
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"UV:%d\r\n",(unsigned int)value_int);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                              
                                if(value_int == 1 && IsUVOn == 0)
                                {
                                    UV_On();  
                                }
                                else if(value_int == 0 && IsUVOn == 1)
                                {
                                    UV_Off();
                                }
                              
                            }                                                        
                            position = strstr(U0RxBuffer,"NI");
                            if(position != NULL)
                            {
                                //ION控制
                                value_int = -1;
                                value_int = string_get_int(position);
                              
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"NI:%d\r\n",(unsigned int)value_int);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));  

                                if(value_int == 1 && Is_ION_On == 0)
                                {
                                    ION_On();  
                                }
                                else if(value_int == 0 && Is_ION_On == 1)
                                {
                                    ION_Off();
                                }                                
                              
                            }
                            position = strstr(U0RxBuffer,"DR");
                            if(position != NULL)
                            {
                                //开仓门
                                value_int = -1;
                                value_int = string_get_int(position);
                              
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"DR:%d\r\n",(unsigned int)value_int);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));  
                                
                                dr_confirm = 1;
                              
                                //改变下一次同步的时间，立刻执行sync动作
                                //next_sync_mstime = get_sys_mstime();
                                next_sync_stime = get_sys_stime();
                                is_fast_sync = 1;
                                fast_sync_start_time = get_sys_stime();
                                
                                Door_Open();                                                        
                            }
                            
                            //if((position = strstr(U0RxBuffer,"EXAMID")) != NULL)
                             
                            position = strstr(U0RxBuffer,"EXAMID");
                            if(position != NULL)
                            {
                                //因为ram空间有限，和CCID公用同一个数组
                                mymemset(ccid,0,sizeof(ccid));
                                strncpy(ccid,position + 9,CHECK_ID_LEN);
                      
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"check_id:%s\r\n",(char *)ccid);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                            }
                    
                            if((position = strstr(U0RxBuffer,"EXAMTYPE")) != NULL)
                            {
                                check_type = string_get_int(position);
                      
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"check_type:%d\r\n",check_type);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                            }
                    
                            if((position = strstr(U0RxBuffer,"EXAMSTATE")) != NULL)
                            {
                                check_state = string_get_int(position);
                      
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"check_state:%d\r\n",check_state);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                            }
                    
                            if(check_type == 1)
                            {
                                //自动检测
                                sys_check_info.check_type = 1;
                                if(check_state == 0)
                                {
                                    if(is_sys_auto_check == 1)
                                    {
                                        stop_sys_auto_check();
                                    }
                                }
                                else if(check_state == 1)
                                {
                                    if(is_sys_auto_check == 0 && is_sys_manual_check == 0)
                                    {
                                        start_sys_auto_check();
                                    }                          
                                }
                            }
                            else if(check_type == 2)
                            {
                                //手动检测
                                sys_check_info.check_type = 2;
                                if(check_state == 0)
                                {
                                    if(is_sys_manual_check == 1)
                                    {
                                        stop_sys_manual_check();
                                    }
                                }
                                else if(check_state == 1)
                                {
                                    if(is_sys_manual_check == 0 && is_sys_auto_check == 0)
                                    {
                                        start_sys_manual_check();
                                    }
                                }
                            }
                            
                        }
                    }
                    
                    if((position = strstr(U0RxBuffer,"CF")) == NULL)
                    {
                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                        sprintf(m26_cmd_info.sendtring,"not find CF\r\n");
                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));                   
                    }
                    else
                    {
                        value_int = -1;
                        value_int = string_get_int(position);
                      
//                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                        sprintf(m26_cmd_info.sendtring,"CF val:%d\r\n",(unsigned int)value_int);
//                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                      
                        if(value_int == 1)
                        {
                            //debug
                            //DEBUG_Uart_Sendbytes(U0RxBuffer,strlen(U0RxBuffer)); 
                            //获取到的字符串中存在筹码信息 
                            position = strstr(U0RxBuffer,"\"C\"");
                            if(position != NULL)
                            {
                                value_long = -1;
                                value_long = string_get_long(position);
                              
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"C:%d\r\n",(unsigned int)value_int);
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                            }
                            
                            position = strstr(U0RxBuffer,"\"T\"");
                            if(position != NULL)
                            {
                                value_int = -1;
                                value_int = string_get_int(position);
                              
//                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                                sprintf(m26_cmd_info.sendtring,"T:%d\r\n",(unsigned int)value_int);
//                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                            }
                            
                            position = strstr(U0RxBuffer,"\"S\"");
                            if(position != NULL)
                            {
                                status = -1;
                                status = string_get_int(position);
                              
//                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                                sprintf(m26_cmd_info.sendtring,"S:%d\r\n",(unsigned int)status);
//                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                            }
                          
                            position = strstr(U0RxBuffer,"\"O\"");
                            if(position != NULL)
                            {
                                mymemset(m26_cmd_info.sendtring,0,strlen(m26_cmd_info.sendtring));
                                sprintf(m26_cmd_info.sendtring,"odernum:");
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                              
                                //ordernum复制到 m26_cmd_info.sendtring 中
                                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                string_get_string(position,m26_cmd_info.sendtring);
                                //debug
                                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                              
                                //立刻对Order num进行比较，防止 m26_cmd_info.sendtring 在后面被改变了
                                //如果比较结果相等 ordnum_cmp_result == 0
                                ordnum_cmp_result = strcmp(order_num,m26_cmd_info.sendtring);
                                if(strlen(m26_cmd_info.sendtring) <= 40)
                                {
                                    strcpy(order_num,m26_cmd_info.sendtring);
                                }
                                else
                                {
                                    mymemset(m26_cmd_info.sendtring,0,strlen(m26_cmd_info.sendtring));
                                    sprintf(m26_cmd_info.sendtring,"odernum too long");
                                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring)); 
                                    
                                    return;
                                }
                                
                                  
                                
                            }
                            
                            if(status == 1)
                            {
                                if(value_int >= 0 && value_long >= 0)
                                {
                                    if(value_int == 0)
                                    {
                                        //把设备变更为家用型
                                        charge_info.IsChargeType = 0;
                                        charge_info.lefttime.l_time = 0;
                                        charge_lefttime_flash_write();
                                    }
                                    else 
                                    {
                                        if(ordnum_cmp_result != 0)
                                        {
                                            Buzzer_Get_Charge();
                                          
                                            if(charge_info.IsChargeType != 1)
                                            {
                                                charge_info.IsChargeType = 1;
                                                charge_info.lefttime.l_time = (unsigned long)value_long;
                                            }
                                            else 
                                            {
                                                charge_info.lefttime.l_time += (unsigned long)value_long;                                           
                                            }
                                            charge_info.next_1min_time = get_sys_stime() + (MIN_TO_S | 0x00);
                                            charge_lefttime_flash_write();
                                            
                                            if(charge_info.IsChargeType == 1 && charge_info.lefttime.l_time > 0)
                                            {
                                                if(sys_mode == STANDBY)
                                                {
                                                    //延时1秒钟，否则接收到筹码开机的两个蜂鸣器响声会重叠在一起
                                                    if(g_1s_flag == 1)
                                                    {
                                                        g_1s_flag = 0;
                                                    }
                                                    while(g_1s_flag == 0);
                                                    g_1s_flag = 0;
                                                    while(g_1s_flag == 0);
                                                    sys_start(); 
                                                    Clear_Touch_Info();
                                                }
                                            }
                                            
                                            is_fast_sync = 1;
                                            
                                            mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                                            sprintf(m26_cmd_info.sendtring,"time:%x %x %x %x\r\n",(unsigned int)charge_info.lefttime.c_time[0],(unsigned int)charge_info.lefttime.c_time[1],
                                                  (unsigned int)charge_info.lefttime.c_time[2],(unsigned int)charge_info.lefttime.c_time[3]);
                                            DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,strlen(m26_cmd_info.sendtring));  

                                            
                                        }
                                    }
                                    //筹码确认
                                    charge_confirm = 1;
                                    //立刻上传一次数据
                                    next_upload_data_time = get_sys_stime() - 1; 
                                    
                                }
                            }
                            
                        }

                    }
                    
                    if(Is_send_dr_confirm == 1)
                    {
                        dr_confirm = 0;
                        Is_send_dr_confirm = 0;
                    }
//                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
//                sprintf(m26_cmd_info.sendtring,"device sync sucess\r\n");
//                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));  

                }   
                else if(M26_BIT_CHECK(m26_action,HTTP_C_CONFIRM))
                {
                    //筹码确认
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"confirm\r\n");
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"%s\r\n",U0RxBuffer);
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                  
                    charge_confirm = 0;
                }
                if(M26_BIT_CHECK(m26_action,HTTP_FACTORY_CHECK))
                {
                    if((position = strstr(U0RxBuffer,"EXAMID")) != NULL)
                    {
                        //因为ram空间有限，和CCID公用同一个数组
                        mymemset(ccid,0,sizeof(ccid));
                        strncpy(ccid,position + 8,CHECK_ID_LEN);
                      
                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                        sprintf(m26_cmd_info.sendtring,"check_id:%s\r\n",(char *)ccid);
                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                    }
                    
                    if((position = strstr(U0RxBuffer,"EXAMTYPE")) != NULL)
                    {
                        check_type = string_get_int(position);
                      
                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                        sprintf(m26_cmd_info.sendtring,"check_type:%d\r\n",check_type);
                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                    }
                    
                    if((position = strstr(U0RxBuffer,"EXAMSTATE")) != NULL)
                    {
                        check_state = string_get_int(position);
                      
                        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                        sprintf(m26_cmd_info.sendtring,"check_state:%d\r\n",check_state);
                        DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                    }
                    
                    if(check_type == 1)
                    {
                        //自动检测
                        sys_check_info.check_type = 1;
                        if(check_state == 0)
                        {
                            if(is_sys_auto_check == 1)
                            {
                                stop_sys_auto_check();
                            }
                        }
                        else if(check_state == 1)
                        {
                            if(is_sys_auto_check == 0 && is_sys_manual_check == 0)
                            {
                                start_sys_auto_check();
                            }                          
                        }
                    }
                    else if(check_type == 2)
                    {
                        //手动检测
                        sys_check_info.check_type = 2;
                        if(check_state == 0)
                        {
                            if(is_sys_manual_check == 1)
                            {
                                stop_sys_manual_check();
                            }
                        }
                        else if(check_state == 1)
                        {
                            if(is_sys_manual_check == 0 && is_sys_auto_check == 0)
                            {
                                start_sys_manual_check();
                            }
                        }
                    }
                    
                    
                   
                }                         
                http_step = 0;
                Is_http = 0;
                m26_action = 0;
                
                //收到筹码后立刻执行筹码确认，防止连续收到
                if(charge_confirm == 1)
                {
                    M26_Charge_Confirm();
                }
            }
            
        }
        else if(m26_cmd_info.cmd_result == CMD_ERROR)
        {
            m26_cmd_info.error_times += 1;
            m26_cmd_info.Is_wait_data = 0;
          
            //按下组合键了，正在检测信号
            if(Is_signal_check == 1)
            {
                signal_check_err_times += 1;
            }
          
            if(Is_http == 1 && (http_step == 1 || http_step == 3))
            {
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"send string error,fail\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                http_step = 0;
                return;
            }
          
          
            if(strcmp(m26_cmd_info.cmd,M26_ATI_CMD) == 0)
            {
                
            }
            else if(strcmp(m26_cmd_info.cmd,M26_CSQ_CMD) == 0)
            {     
                //获取信号质量             
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"get signal quality error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QCCID_CMD) == 0)
            {     
                //读取SIM 卡CCID          
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QCCID_CMD error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_GSN_CMD) == 0)
            {
                //获取IMEI失败              
                //Is_Get_IMEI = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QSPN_CMD) == 0)
            {     
                //读取SIM 卡服务运营商名称             
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QSPN_CMD error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QIFGCNT_CMD) == 0)
            {     
                //设置前景模式              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QIFGCNT_CMD error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QICSGP_CM_CMD) == 0)
            {   
                //设置链接模式              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QICSGP_CM_CMD error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                //M26_register_net.Is_m26_registerd = 0;
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QIREGAPP_CMD) == 0 || strcmp(m26_cmd_info.cmd,M26_QIACT_CMD) == 0)
            {   
                //启动任务并设置接入点   
                if(strcmp(m26_cmd_info.cmd,M26_QIREGAPP_CMD) == 0)
                {
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"M26_QIREGAPP_CMD error\n");
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                } 
                else
                {
                //激活网络             
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"M26_QIACT_CMD error\n");
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));                
                }                  

                M26_Restart();
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QHTTPURL_CMD) == 0)
            {   
                //set HTTP Server URL              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QHTTPURL_CMD error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                //M26_register_net.Is_m26_registerd = 0;
                http_step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QHTTPPOST_CMD) == 0)
            {   
                //http post             
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QHTTPPOST_CMD error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                //M26_register_net.Is_m26_registerd = 0;
                http_step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QHTTPREAD_CMD) == 0)
            {   
                //http read             
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QHTTPREAD_CMD error\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                //M26_register_net.Is_m26_registerd = 0;
                http_step = 0;
            }
            
             //如果是状态更新，则重新把所有状态上传
            if(M26_BIT_CHECK(m26_action,HTTP_SYNC) )  
            {
                //如果本次是device sync，但是失败了，则下一次仍然上传设备状态
                if(sync_this_loop == 1)
                {
                    resync = 1;
                }
                
                //如果发送DR失败，是否有必要再次发送DR，有需验证，再次发送可能已经超时
//                if(dr_confirm == 1)
//                {
//                    Is_send_dr_confirm = 0;
//                }
                  dr_confirm = 0;
                  Is_send_dr_confirm = 0;
            }
          
          
        }
        else if(m26_cmd_info.cmd_result == CMD_TIMEOUT)
        {
            m26_cmd_info.timeout_times += 1;
            m26_cmd_info.Is_wait_data = 0;
          
            //按下组合键了，正在检测信号
            if(Is_signal_check == 1)
            {
                signal_check_err_times += 1;
            }
          
            if(Is_http == 1)
            {
                http_step = 0;
            }
            
            if(strcmp(m26_cmd_info.cmd,M26_CSQ_CMD) == 0)
            {     
                //获取信号质量            
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"get signal quality timeout\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 0;
            }
            else if(strcmp(m26_cmd_info.cmd,M26_QCCID_CMD) == 0)
            {     
                //读取SIM 卡CCID            
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                sprintf(m26_cmd_info.sendtring,"M26_QCCID_CMD timeout\n");
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
              
                M26_register_net.step = 0;
            }
            
            if(strcmp(m26_cmd_info.cmd,M26_QIREGAPP_CMD) == 0 || strcmp(m26_cmd_info.cmd,M26_QIACT_CMD) == 0 || strcmp(m26_cmd_info.cmd,M26_QCCID_CMD) == 0)
            {   
                //启动任务并设置接入点   
                if(strcmp(m26_cmd_info.cmd,M26_QIREGAPP_CMD) == 0)
                {
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"M26_QIREGAPP_CMD timeout\n");
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
                } 
                else
                {
                //激活网络             
                    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
                    sprintf(m26_cmd_info.sendtring,"M26_QIACT_CMD timeout\n");
                    DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));                
                }                  

                M26_Restart();
                
            }
            
            ////如果本次是device sync，但是失败了，则下一次仍然上传设备状态
            if(M26_BIT_CHECK(m26_action,HTTP_SYNC))  
            {
                if(sync_this_loop == 1)
                {
                    resync = 1;
                }
                
                //如果发送DR失败，是否有必要再次发送DR，有需验证，再次发送可能已经超时
//                if(dr_confirm == 1)
//                {
//                    Is_send_dr_confirm = 0;
//                }
                  dr_confirm = 0;
                  Is_send_dr_confirm = 0;
            }
        }
        
    }

}



//每次发送命令前先把UART0的缓存区清空
void clear_wifi_uart_buff(void)
{
    mymemset(U0RxBuffer,0,U0RxPtr);
    U0RxPtr = 0;

}


int M26_Send_data(unsigned char type,const char *cmd,const char *str)
{
    //unsigned char temp = 0;
    unsigned char debug_buff[30] = {0};

  
    if(m26_cmd_info.Is_wait_data == 1)
    {
        return M26_BUSY;
    }
  
    if(str == NULL || str[0] == 0)
    {
        return M26_SEND_DATA_NULL;
    }
    
    clear_wifi_uart_buff();
    
    mymemset(m26_cmd_info.end_flag,0,sizeof(m26_cmd_info.end_flag));
    
    m26_cmd_info.cmd_result = CMD_WAIT;
    

    
    Wifi_Uart_Sendbytes(str,(unsigned int)strlen(str));
    
    m26_cmd_info.cmd_send_time = get_sys_mstime();
    
    
    if(type == CMD_TYPE)
    {
      
        mymemset(debug_buff,0,sizeof(debug_buff));
        sprintf(debug_buff,"cmd:%s\n",m26_cmd_info.cmd);
        DEBUG_Uart_Sendbytes(debug_buff,strlen(debug_buff));
        
    }
    else
    {
        //m26_cmd_info.cmd = NULL;
    }
    
    
    if(type == STRING_TYPE)
    {
        strcpy(m26_cmd_info.end_flag,END_OK);
    }
    else
    {      
        if(strcmp(cmd,M26_ATI_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        if(strcmp(cmd,M26_CSQ_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_GSN_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_QCCID_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_QSPN_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_QIFGCNT_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_QICSGP_CM_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_QIREGAPP_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_QIACT_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
        else if(strcmp(cmd,M26_QHTTPURL_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_CONNECT);
        }
        else if(strcmp(cmd,M26_QHTTPPOST_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_CONNECT);
        }
        else if(strcmp(cmd,M26_QHTTPREAD_CMD) == 0)
        {
            strcpy(m26_cmd_info.end_flag,END_OK);
        }
    }

    
  //此处需设置
    m26_cmd_info.Is_wait_data = 1;
    
    
    return M26_OK;


}


void M26_Register_to_Net(void)
{
    unsigned char res = 0;

    
    //unsigned char debug_buff[50] = {0};
    
  
    //如果在等待上一条指令的数据返回，则直接返回
    if(m26_cmd_info.Is_wait_data == 1)
    {
        return;
    }
    
    if(M26_register_net.Is_m26_registerd == 1)
    {
        return;
    }
    

    mymemset(m26_cmd_info.cmd,0,sizeof(m26_cmd_info.cmd));
    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
 
    if(M26_register_net.step == 0)
    {    
        m26_cmd_info.type = CMD_TYPE;      
        strcpy(m26_cmd_info.cmd,M26_QSPN_CMD);   
         
    }
    else if(M26_register_net.step == 1)
    {
        m26_cmd_info.type = CMD_TYPE;   
        strcpy(m26_cmd_info.cmd,M26_QIFGCNT_CMD);     
    } 
    else if(M26_register_net.step == 2)
    {   
        m26_cmd_info.type = CMD_TYPE;         
        strcpy(m26_cmd_info.cmd,M26_QICSGP_CM_CMD);      
    }
    else if(M26_register_net.step == 3)
    { 
        //启动任务并设置接入点 
        m26_cmd_info.type = CMD_TYPE;         
        strcpy(m26_cmd_info.cmd,M26_QIREGAPP_CMD);      
    }
    else if(M26_register_net.step == 4)
    {      
        //激活网络
        m26_cmd_info.type = CMD_TYPE;   
        strcpy(m26_cmd_info.cmd,M26_QIACT_CMD);       \
    }
    
    m26_cmd_info.type = CMD_TYPE;     
    
    res = make_m26_send_string(m26_cmd_info.sendtring,m26_cmd_info.cmd,NULL);     
    if(res == 0)
    {
        M26_Send_data(m26_cmd_info.type,m26_cmd_info.cmd,m26_cmd_info.sendtring);
    } 
   
   //M26_Send_data(CMD_TYPE,m26_cmd_info.cmd,m26_cmd_info.sendtring);

//    mymemset(debug_buff,0,sizeof(debug_buff));
//    sprintf(debug_buff,"cmdstr:%s\n",data_string);
//    DEBUG_Uart_Sendbytes(debug_buff,strlen(debug_buff));    

}


void m26_http_loop(void)
{
    unsigned char res = 0;
    unsigned char string_length = 0;
    
  
    if(Is_http == 0)
    {
        return;
    }
    
    
    if(m26_cmd_info.Is_wait_data == 1)
    {
        return;
    }
    
    mymemset(m26_cmd_info.cmd,0,sizeof(m26_cmd_info.cmd));
    mymemset(m26_cmd_info.cmd_para,0,sizeof(m26_cmd_info.cmd_para));
    if(http_step != 3)
    {
        mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));    
    }

    
    //if(m26_action & (1 << HTTP_ACTIVATE))
    if(M26_BIT_CHECK(m26_action,HTTP_ACTIVATE))
    {
        if(http_step == 0)
        {
            //set HTTP Server URL
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPURL_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_ACTIVATE_URL_PAR);
        }
        else if(http_step == 1)
        {
            //send http address string
            m26_cmd_info.type = STRING_TYPE;
            strcpy(m26_cmd_info.sendtring,SERVER_ADDR);            
            //device activate
            strcat(m26_cmd_info.sendtring,M26_ACTIVATE_URL);           
        }
        else if(http_step == 2 || http_step == 3)
        {
            //http post
            sprintf(m26_cmd_info.sendtring,"PID=%s&CCID=%s&PW=%d",PRODUCT_ID,ccid,(unsigned int)(g_sys_time_ms%10000));  
          
            //DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,strlen(m26_cmd_info.sendtring));

            if(http_step == 2)
            {
                m26_cmd_info.type = CMD_TYPE;
                strcpy(m26_cmd_info.cmd,M26_QHTTPPOST_CMD);
              
                string_length = (unsigned char)strlen(m26_cmd_info.sendtring);   
                sprintf(m26_cmd_info.cmd_para,M26_POST_PARA,(unsigned int)string_length); 
              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));    
            }
            else
            {
                m26_cmd_info.type = STRING_TYPE;
            }                          
        }
        else if(http_step == 4)
        {
            //http read
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPREAD_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_HTTPREAD_PARA);
        }
    }
    else if(M26_BIT_CHECK(m26_action,HTTP_OTA))           //ota check
    {
        if(http_step == 0)
        {
            //set HTTP Server URL
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPURL_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_OTA_CHECK_URL_PAR);
        }
        else if(http_step == 1)
        {
            //send http address string
            m26_cmd_info.type = STRING_TYPE;
            strcpy(m26_cmd_info.sendtring,SERVER_ADDR);            
            //ota check
            strcat(m26_cmd_info.sendtring,M26_OTA_CHECK_URL);  
            
            //debug
            DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,strlen(m26_cmd_info.sendtring));
        }
        else if(http_step == 2 || http_step == 3)
        {
            //http post
            sprintf(m26_cmd_info.sendtring,"DID=%s&MN=M26&FW=%s&IV=%s",device_id,FIRMWARE_VERSION,IOT_VERSION);  
          
            

            if(http_step == 2)
            {
                m26_cmd_info.type = CMD_TYPE;
                strcpy(m26_cmd_info.cmd,M26_QHTTPPOST_CMD);
              
                string_length = (unsigned char)strlen(m26_cmd_info.sendtring);   
                sprintf(m26_cmd_info.cmd_para,M26_POST_PARA,(unsigned int)string_length); 
              
                //debug
                //sprintf(m26_cmd_info.sendtring,"length:%d\r\n",(unsigned int)string_length); 
                //DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,strlen(m26_cmd_info.sendtring));              
              
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));    
            }
            else
            {
                m26_cmd_info.type = STRING_TYPE;
                //debug
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,strlen(m26_cmd_info.sendtring));
            }                          
        }
        else if(http_step == 4)
        {
            //http read
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPREAD_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_HTTPREAD_PARA);
        }    
    }
    else if(M26_BIT_CHECK(m26_action,HTTP_UPLOAD_DATA))    //upload data
    {
        if(http_step == 0)
        {
            //set HTTP Server URL
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPURL_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_UPLOAD_DATA_URL_PAR);
        }
        else if(http_step == 1)
        {
            //send http address string
            m26_cmd_info.type = STRING_TYPE;
            strcpy(m26_cmd_info.sendtring,SERVER_ADDR);            
            //device activate
            strcat(m26_cmd_info.sendtring,M26_UPLOAD_DATA_URL);           
        }
        else if(http_step == 2 || http_step == 3)
        {
            //http post          
            if(http_step == 2)
            {
                charge_info.lefttime_bak = charge_info.lefttime.l_time;
            }
            
            sprintf(m26_cmd_info.sendtring,"DID=%s&SF=1&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d,\"TYPE\":%d,\"CHIPS\":%d,\"P\":%d"
                          ,device_id,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
                           (unsigned int)Is_ION_On,(unsigned int)charge_info.IsChargeType,(unsigned int)charge_info.lefttime_bak,
                             (unsigned int)display_pm_value);

            if(http_step == 2)
            {
                m26_cmd_info.type = CMD_TYPE;
                strcpy(m26_cmd_info.cmd,M26_QHTTPPOST_CMD);
          
//            sprintf(m26_cmd_info.sendtring,"DID=%s&SF=%d&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d,\"T\":%d,\"C\":%d"
//                          ,device_id,0,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
//                           (unsigned int)Is_ION_On,(unsigned int)0,(unsigned int)0); 
              
                string_length = (unsigned char)strlen(m26_cmd_info.sendtring);   
                sprintf(m26_cmd_info.cmd_para,M26_POST_PARA,(unsigned int)string_length); 
    
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));  
            } 
            else
            {
                m26_cmd_info.type = STRING_TYPE;
                //调试
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));  
            }              
                 
        }
        else if(http_step == 4)
        {
            //http read
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPREAD_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_HTTPREAD_PARA);
        }
    }
    else if(M26_BIT_CHECK(m26_action,HTTP_SYNC))        //status sync
    {
        if(http_step == 0)
        {
            //set HTTP Server URL
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPURL_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_STATUS_SYNC_URL_PAR);
        }
        else if(http_step == 1)
        {
            //send http address string
            m26_cmd_info.type = STRING_TYPE;
            strcpy(m26_cmd_info.sendtring,SERVER_ADDR);            
            strcat(m26_cmd_info.sendtring,M26_STATUS_SYNC_URL);           
        }
        else if(http_step == 2 || http_step == 3)
        {
            //http post   
            if(http_step == 2)
            {
                charge_info.lefttime_bak = charge_info.lefttime.l_time;
              
                //sync_this_loop的状态在http的一个周期内只更新一次
                if(dev_status == 1)
                {
                    sync_this_loop = 1;
                    dev_status = 0;
                }          
                else
                {  
                    sync_this_loop = 0;
                }
                
                //如果上一次sync失败，则这次仍然上传设备状态
                if(resync == 1)
                {
                    sync_this_loop = 1;
                }
                
            }

//            if(dr_confirm == 1)
//            {
//                sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"DR\":1",device_id); 
//                Is_send_dr_confirm = 1;
//            }
//            else
//            {
//            sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d,\"TYPE\":%d,\"CHIPS\":%d,\"P\":%d"
//                          ,device_id,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
//                           (unsigned int)Is_ION_On,(unsigned int)charge_info.IsChargeType,(unsigned long)charge_info.lefttime_bak,
//                            (unsigned int)display_pm_value);            
//            }


                                         

            if(sync_this_loop == 1)
            {
                if(dr_confirm == 1)
                {
                    sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d,\"DR\":1"
                          ,device_id,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
                           (unsigned int)Is_ION_On);                     
                }
                else
                {
                    sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d"
                          ,device_id,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
                           (unsigned int)Is_ION_On);                 
                }
                      
            }
            else
            {
                if(dr_confirm == 1)
                {
                    //sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"DR\":1",device_id);      
                  
                    sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d,\"TYPE\":%d,\"CHIPS\":%d,\"P\":%d,\"DR\":1"
                          ,device_id,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
                           (unsigned int)Is_ION_On,(unsigned int)charge_info.IsChargeType,(unsigned int)charge_info.lefttime_bak,
                            (unsigned int)display_pm_value);  
                }
                else
                {
                    //sprintf(m26_cmd_info.sendtring,"DID=%s",device_id); 

                    sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d,\"TYPE\":%d,\"CHIPS\":%d,\"P\":%d"
                          ,device_id,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
                           (unsigned int)Is_ION_On,(unsigned int)charge_info.IsChargeType,(unsigned int)charge_info.lefttime_bak,
                            (unsigned int)display_pm_value);                    
                }                         
            }

             

            if(http_step == 2)
            {
                
              
                m26_cmd_info.type = CMD_TYPE;
                strcpy(m26_cmd_info.cmd,M26_QHTTPPOST_CMD);
              
//                sprintf(m26_cmd_info.sendtring,"DID=%s&SD=\"SMD\":%d,\"SM\":%d,\"FA\":%d,\"UV\":%d,\"NI\":%d,\"T\":%d,\"C\":%d"
//                          ,device_id,(unsigned int)sys_mode,(unsigned int)run_mode,(unsigned int)speed_dang,(unsigned int)IsUVOn,
//                           (unsigned int)Is_ION_On,(unsigned int)0,(unsigned int)0); 
              
                string_length = (unsigned char)strlen(m26_cmd_info.sendtring);   
                sprintf(m26_cmd_info.cmd_para,M26_POST_PARA,(unsigned int)string_length); 
    
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));  
          
            } 
            else
            {
                m26_cmd_info.type = STRING_TYPE;
                //调试
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));  
            }              
                 
        }
        else if(http_step == 4)
        {
            //http read
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPREAD_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_HTTPREAD_PARA);
        }    
    }
    else if(M26_BIT_CHECK(m26_action,HTTP_C_CONFIRM))     //charge confirm
    {
        if(http_step == 0)
        {
            //set HTTP Server URL
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPURL_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_C_CONFIRM_URL_PAR);
        }
        else if(http_step == 1)
        {
            //send http address string
            m26_cmd_info.type = STRING_TYPE;
            strcpy(m26_cmd_info.sendtring,SERVER_ADDR);            
            strcat(m26_cmd_info.sendtring,M26_C_CONFIRM_URL);           
        }
        else if(http_step == 2 || http_step == 3)
        {
            //http post          
            sprintf(m26_cmd_info.sendtring,"DID=%s&ORN=%s",device_id,order_num); 
             
            if(http_step == 2)
            {
                m26_cmd_info.type = CMD_TYPE;
                strcpy(m26_cmd_info.cmd,M26_QHTTPPOST_CMD);
              
                string_length = (unsigned char)strlen(m26_cmd_info.sendtring);   
                sprintf(m26_cmd_info.cmd_para,M26_POST_PARA,(unsigned int)string_length); 
    
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));  
          
            } 
            else
            {
                m26_cmd_info.type = STRING_TYPE;
            }              
                 
        }
        else if(http_step == 4)
        {
            //http read
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPREAD_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_HTTPREAD_PARA);
        }    
    }
    else if(M26_BIT_CHECK(m26_action,HTTP_FACTORY_CHECK))    
    {
        if(http_step == 0)
        {
            //set HTTP Server URL
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPURL_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_FACTORY_CHECK_URL_PAR);
        }
        else if(http_step == 1)
        {
            //send http address string
            m26_cmd_info.type = STRING_TYPE;
            strcpy(m26_cmd_info.sendtring,SERVER_ADDR);            
            strcat(m26_cmd_info.sendtring,M26_FACTORY_CHECK_URL);           
        }
        else if(http_step == 2 || http_step == 3)
        {
            //http post 
            if(sys_check_info.check_type == 1)
            {
                //自动
                sprintf(m26_cmd_info.sendtring,"EID=%s&ED=DOOR:%d,FAN:%d,UV:%d,PM25S:%d,PM25:%d",ccid,(unsigned int)AUTO_CHECK_1(sys_check_info.status,door_bit),(unsigned int)AUTO_CHECK_1(sys_check_info.status,fan_bit),
                        (unsigned int)AUTO_CHECK_1(sys_check_info.status,uv_bit),(unsigned int)AUTO_CHECK_1(sys_check_info.status,pm25_bit),(unsigned int)PM25_value); 
                
                
//                sprintf(m26_cmd_info.sendtring,"EID=%s&ED=\"DR\":%d,FAN:%d,UV:%d,PM25S:%d,PM25:%d",ccid,(unsigned int)AUTO_CHECK_1(sys_check_info.status,door_bit),(unsigned int)AUTO_CHECK_1(sys_check_info.status,fan_bit),
//                        (unsigned int)AUTO_CHECK_1(sys_check_info.status,uv_bit),(unsigned int)AUTO_CHECK_1(sys_check_info.status,pm25_bit),(unsigned int)PM25_value); 
            }
            else
            {
                //手动
                sprintf(m26_cmd_info.sendtring,"EID=%s&ED=PWK:%d,QK:%d,LK:%d,MK:%d,HK:%d,TK:%d,MDK:%d",ccid,(unsigned int)MAN_CHECK_1(sys_check_info.touch_key_check,power_bit),(unsigned int)MAN_CHECK_1(sys_check_info.touch_key_check,quiet_speed_bit),
                        (unsigned int)MAN_CHECK_1(sys_check_info.touch_key_check,low_speed_bit),(unsigned int)MAN_CHECK_1(sys_check_info.touch_key_check,mid_speed_bit),(unsigned int)MAN_CHECK_1(sys_check_info.touch_key_check,high_speed_bit),
                          (unsigned int)MAN_CHECK_1(sys_check_info.touch_key_check,timer_bit),(unsigned int)MAN_CHECK_1(sys_check_info.touch_key_check,mode_bit)); 
                        
            }
            
             
            if(http_step == 2)
            {
                m26_cmd_info.type = CMD_TYPE;
                strcpy(m26_cmd_info.cmd,M26_QHTTPPOST_CMD);
              
                string_length = (unsigned char)strlen(m26_cmd_info.sendtring);   
                sprintf(m26_cmd_info.cmd_para,M26_POST_PARA,(unsigned int)string_length); 
    
                mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));  
          
            } 
            else
            {
                m26_cmd_info.type = STRING_TYPE;
               
                //debug
                DEBUG_Uart_Sendbytes(m26_cmd_info.sendtring,mystrlen(m26_cmd_info.sendtring));
            }              
                 
        }
        else if(http_step == 4)
        {
            //http read
            m26_cmd_info.type = CMD_TYPE;
            strcpy(m26_cmd_info.cmd,M26_QHTTPREAD_CMD);
            strcpy(m26_cmd_info.cmd_para,M26_HTTPREAD_PARA);
        }    
    }
    
    res = make_m26_send_string(m26_cmd_info.sendtring,m26_cmd_info.cmd,m26_cmd_info.cmd_para);    
    if(res == 0)
    {
        M26_Send_data(m26_cmd_info.type,m26_cmd_info.cmd,m26_cmd_info.sendtring);
    } 


}

/*
void device_register_to_net(void)
{
  
  while(M26_register_net.Is_m26_registerd == 0)
  {
      if(_test_timeflag(g_1s_flag))
      {
          g_1s_flag = 0;
          if(Is_signal_err_code_zero == 1)
          {
              M26_Register_to_Net(); 
          }
          else
          {
              M26_Get_Signal_Quality();
          }
          
      }
      if(_test_timeflag(g_100ms_flag))
      {    
                   
          M26_wait_response(); 
          check_m26_cmd_result();        
      }
  } 

}
*/

void M26_Get_Signal_Quality(void)
{
    unsigned char res = 0;
  
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    mymemset(m26_cmd_info.cmd,0,sizeof(m26_cmd_info.cmd));
    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
   
    m26_cmd_info.type = CMD_TYPE;      
    strcpy(m26_cmd_info.cmd,M26_CSQ_CMD);   
             
    res = make_m26_send_string(m26_cmd_info.sendtring,m26_cmd_info.cmd,NULL);     
    if(res == 0)
    {
        M26_Send_data(m26_cmd_info.type,m26_cmd_info.cmd,m26_cmd_info.sendtring);
    } 
    
}

/*
//当M26刚启动时，先读取信号质量，如果错误码率不等于0的情况下激活网络会失败
void device_get_signal_quality(void)
{
  while(Is_signal_err_code_zero != 1)
  {
      if(_test_timeflag(g_1s_flag))
      {
          g_1s_flag = 0;          
          M26_Get_Signal_Quality();
      }
      if(_test_timeflag(g_100ms_flag))
      {                       
          M26_wait_response(); 
          check_m26_cmd_result();   
          if(Is_signal_err_code_zero == 1)
          {
              break;
          }
      }
  }
}
*/

void M26_Get_CCID(void)
{
    unsigned char res = 0;
  
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    mymemset(m26_cmd_info.cmd,0,sizeof(m26_cmd_info.cmd));
    mymemset(m26_cmd_info.sendtring,0,sizeof(m26_cmd_info.sendtring));
   
    m26_cmd_info.type = CMD_TYPE;      
    strcpy(m26_cmd_info.cmd,M26_QCCID_CMD);   
             
    res = make_m26_send_string(m26_cmd_info.sendtring,m26_cmd_info.cmd,NULL);     
    if(res == 0)
    {
        M26_Send_data(m26_cmd_info.type,m26_cmd_info.cmd,m26_cmd_info.sendtring);
    } 
    
}


void M26_Device_Activate(void)
{
   
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    m26_action = 0;
    m26_action |= (1 << HTTP_ACTIVATE);
    
    Is_http = 1;
}


void device_activate(void)
{
  M26_Device_Activate();    
  while(Is_device_activate == 0)
  {
      if(_test_timeflag(g_100ms_flag))
      {    
          m26_http_loop();        
          M26_wait_response(); 
          check_m26_cmd_result();        
      }
  } 

}

void M26_Ota_Check(void)
{
    if(Is_device_activate == 0)
    {
        return;
    } 
    
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    m26_action = 0;
    m26_action |= (1 << HTTP_OTA);
    
    Is_http = 1;
}


#define UPLOAD_DATA_TIME_INTERVAL   300
void M26_Upload_Data(void)
{   
    if(is_m26_fogcloud_init == 0)
    {
        return;
    }      
  
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    nowtime_s = get_sys_stime();
    if(nowtime_s >= next_upload_data_time)
    {
        m26_action = 0;
        m26_action |= (1 << HTTP_UPLOAD_DATA);
    
        Is_http = 1; 

        next_upload_data_time = nowtime_s + (UPLOAD_DATA_TIME_INTERVAL | 0x00);      
    }    
}

/*
void M26_Status_Sync(void)
{
    //5秒的时间间隔
    const unsigned long code sync_mstime_interval = 5000;

  
    if(is_m26_fogcloud_init == 0)
    {
        return;
    } 
    
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    nowtime_ms = get_sys_mstime();
    //防止时间溢出
    if(next_sync_mstime > 0xF0000000 && nowtime_ms < 0xE0000000)
    {
        next_sync_mstime = nowtime_ms;
    }
    
    
    if(nowtime_ms < next_sync_mstime)
    {
        return;
    }
    
    m26_action = 0;
    m26_action |= (1 << HTTP_SYNC);
    
    Is_http = 1;
    
    next_sync_mstime = nowtime_ms + sync_mstime_interval;
}
*/



void M26_Status_Sync(void)
{
    //5秒的时间间隔
    //const unsigned long code sync_mstime_interval = 30000;
    const unsigned long code sync_short_time_interval = 5;
    const unsigned long code sync_long_time_interval = 30;
    const unsigned long code fast_sync_continue_time = 180;
  


  
    if(is_m26_fogcloud_init == 0)
    {
        return;
    } 
    
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    nowtime_s = get_sys_stime();
    //防止时间溢出
    if(next_sync_stime > 0xF0000000 && nowtime_s < 0xE0000000)
    {
        next_sync_stime = nowtime_s;
    }
    
    
    if(nowtime_s < next_sync_stime)
    {
        return;
    }
    
    m26_action = 0;
    m26_action |= (1 << HTTP_SYNC);
    
    Is_http = 1;
    
    if(nowtime_s >= (fast_sync_start_time + fast_sync_continue_time) && charge_info.lefttime.l_time == 0)
    {
        if(is_fast_sync == 1)
        {
            is_fast_sync = 0;
        }
        
    }
    
    if(is_fast_sync == 1)
    {
        next_sync_stime = nowtime_s + sync_short_time_interval;
    }
    else 
    {
        next_sync_stime = nowtime_s + sync_long_time_interval;
    }
    
}



void M26_Charge_Confirm(void)
{
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return;
    }
    
    m26_action = 0;
    m26_action |= (1 << HTTP_C_CONFIRM);
    
    Is_http = 1;
}

unsigned char M26_Upload_Factory_Check_Result(void)
{
    if(m26_cmd_info.Is_wait_data == 1 || Is_http == 1)
    {
        return M26_BUSY;
    }
    
    m26_action = 0;
    m26_action |= (1 << HTTP_FACTORY_CHECK);
    
    Is_http = 1;
    
    return M26_OK;
}

/*
void device_get_ccid(void)
{
  while(Is_Get_CCID == 0)
  {
      if(_test_timeflag(g_1s_flag))
      {
          g_1s_flag = 0;          
          M26_Get_CCID();
      }
      if(_test_timeflag(g_100ms_flag))
      {                       
          M26_wait_response(); 
          check_m26_cmd_result();        
      }
  } 

}
*/


int string_get_int(const char *string)
{
    unsigned char index_start = 0;
    unsigned char index_end = 0;
    unsigned char i = 0;
  
    //char buff[6] = {0};
  
    for(i = 0;i < 15;i ++)
    {
        if(*(string + i) == ASCII_COLONS)
        {
            if(*(string + i + 1) != ASCII_QUOTES)
            {
                index_start = i + 1;
            }
            else
            {
                index_start = i + 2;
            }
            
        }
        if(*(string + i) == ASCII_COMMA || *(string + i) == ASCII_BRANCE_CLOSE)
        {
            if(*(string + i - 1) != ASCII_QUOTES)
            {
                index_end = i - 1;
            }
            else
            {
                index_end = i - 2;
            }
            
            break;
        }
    }
    
    if(index_end < index_start)
    {
       return -1; 
    }
    else
    {
        if((index_end - index_start) > 5)
        {
            return -1;
        }
    }
    
    mymemset(m26_cmd_info.cmd_para,0,sizeof(m26_cmd_info.cmd_para));
    strncpy(m26_cmd_info.cmd_para,(string + index_start),(index_end - index_start + 1));
    
    
    return(atoi(m26_cmd_info.cmd_para));

}

long string_get_long(const char *string)
{
    unsigned char index_start = 0;
    unsigned char index_end = 0;
    unsigned char i = 0;
  
    //char buff[6] = {0};
  
    for(i = 0;i < 15;i ++)
    {
        if(*(string + i) == ASCII_COLONS)
        {
            index_start = i + 1;
        }
        if(*(string + i) == ASCII_COMMA || *(string + i) == ASCII_BRANCE_CLOSE)
        {
            index_end = i - 1;
            break;
        }
    }
    
    if(index_end < index_start)
    {
       return -1; 
    }
    else
    {
        if((index_end - index_start) > 5)
        {
            return -1;
        }
    }
    
    mymemset(m26_cmd_info.cmd_para,0,sizeof(m26_cmd_info.cmd_para));
    strncpy(m26_cmd_info.cmd_para,(string + index_start),(index_end - index_start + 1));
    
    
    return(atol(m26_cmd_info.cmd_para));

}


void string_get_string(const char *string,char *dest)
{
    //该函数中不要使用m26_cmd_info.string，ordernum复制到了该数组中
    unsigned char index_start = 0;
    unsigned char index_end = 0;
    unsigned char i = 0;

    for(i = 0;i < 50;i ++)
    {
        if(*(string + i) == ASCII_COLONS)
        {
            index_start = i + 2;
        }
        if(*(string + i) == ASCII_QUOTES && (*(string + i + 1) == ASCII_BRANCE_CLOSE || *(string + i + 1) == ASCII_COMMA))
        {
            index_end = i - 1;
            break;
        }
    }
    


    if(index_end < index_start || i == 50)
    {
        return;
    }
    else
    {
        if((index_end - index_start) > 40)
        {
            return;
        }
    }
    mymemset(dest,0,strlen(dest));
    strncpy(dest,(string + index_start),(index_end - index_start + 1));  
    
//    mymemset(m26_cmd_info.cmd,0,sizeof(m26_cmd_info.cmd));
//    sprintf(m26_cmd_info.cmd,"s:%d,t:%d,l:%d\r\n",(unsigned int)index_start,(unsigned int)index_end,(unsigned int)(index_end - index_start + 1));
//    DEBUG_Uart_Sendbytes(m26_cmd_info.cmd,mystrlen(m26_cmd_info.cmd));  
    
    //DEBUG_Uart_Sendbytes((string + index_start),(index_end - index_start + 1));  

  
    
}


void Wifi_Led_On(void)
{
    if(wifi_led_state == 0)
    {
        TM1620_LED_Control(LED_WIFI,LED_ON);
        wifi_led_state = 1;
    }
}

void Wifi_Led_Off(void)
{
    if(wifi_led_state == 1)
    {
        TM1620_LED_Control(LED_WIFI,LED_OFF);
        wifi_led_state = 0;
    }
}

//检测m26是否出现通讯错误，如果一直错误或者超时则重启m26,m26通讯指示灯控制
//此函数不需要太频繁的执行
void m26_fogcloud_init_and_check(void)
{
    if((m26_cmd_info.error_times >= 20 || m26_cmd_info.timeout_times >= 3))
    {
        //重启m26       
        M26_Restart();
        
    }
  
    if(m26_cmd_info.error_times != 0 || m26_cmd_info.timeout_times != 0 || Is_device_activate == 0)
    {
        if(wifi_led_state == 1)
        {
            //关闭wifi led
            Wifi_Led_Off();
        }
    }
    else if(m26_cmd_info.error_times == 0 && m26_cmd_info.timeout_times == 0 && Is_device_activate == 1)
    {
        if(wifi_led_state == 0)
        {
            //打开wifi led
            Wifi_Led_On();
        }
    }
    
    if(Is_Get_CCID == 0)
    {
        M26_Get_CCID();
    }
    
    if(Is_Get_CCID == 1 && Is_signal_err_code_zero == 0)
    {
        M26_Get_Signal_Quality();
    }
    
    if(M26_register_net.Is_m26_registerd == 0 && Is_signal_err_code_zero == 1)
    {
        M26_Register_to_Net();
    }
    
    if(M26_register_net.Is_m26_registerd == 1 && Is_device_activate == 0)
    {
        M26_Device_Activate();
    }
    
    if(is_m26_fogcloud_init == 0 && Is_device_activate == 1)
    {
        M26_Ota_Check();
    }
    
    
}


/*
void m26_fog_service_init(void)
{
    m26_cmd_info.error_times = 0;
    m26_cmd_info.timeout_times = 0;
  
    while(Is_device_activate == 0)
    {
        
        if(_test_timeflag(g_1ms_flag))
        {
            g_1ms_flag = 0;
            sys_run();
            DirectMotor();       
        }
        if(_test_timeflag(g_2ms_flag))
        {
            g_2ms_flag = 0;
            TurnMotor();
        }         
        
  
        if(_test_timeflag(g_10ms_flag))
        {
            g_10ms_flag = 0;
           
            Scan_TouchPad(); 
        
            m26_http_loop();        
            M26_wait_response(); 
            check_m26_cmd_result();
        }
        
        if(_test_timeflag(g_100ms_flag))
        {
            g_100ms_flag = 0;
            Dcmoto_adj();   
            UV_Check();                 
        }
        
        if(_test_timeflag(g_1s_flag))
        {
            g_1s_flag = 0;
            m26_check();
          
            charge_lefttime_count();  
            Check_If_Close_Door();
        }
    }

}
*/

 
#define SIGNAL_CHECK_TIME_INTERVAL   61
//正常情况下5秒钟通讯一次，1分钟内可以通讯11-12次
//为了节省内存空间，此函数不再另外定义long类型的时间变量，此函数放在main函数中的1s部分，此函数一秒执行一次即可
void m26_signal_check(void)
{
    //记录检测了多长时间，单位是秒，暂定检测1分钟。如果检测时间长的话需要修改变量类型
    static unsigned long signal_check_stimes = 0;
    if(Is_signal_check == 0)
    {
        signal_check_stimes = 0;
        signal_check_err_times = 0;
        return;
    }
    
//    if(M26_register_net.Is_m26_registerd == 0 || Is_device_activate == 0)
//    {
//        //此时可能是因为设备连续通讯失败，正在重启
//        return;    
//    }
    
    signal_check_stimes += 1;
    
    TM1620_DispalyData(2,signal_check_err_times);
    
    if(signal_check_stimes <= 2 || signal_check_stimes >= (SIGNAL_CHECK_TIME_INTERVAL - 1))
    {
        Buzzer_Signal_Check();
    }
    
    if(signal_check_stimes >= SIGNAL_CHECK_TIME_INTERVAL)
    {
        Is_signal_check = 0;
    }
    

    
    
}





