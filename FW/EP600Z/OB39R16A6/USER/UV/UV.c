#include <stdio.h>
#include "UV.h"
#include "ADC.h"
#include "global.h"
#include "wifi_uart.h"
#include "common.h"
#include "TM1620.h"


void UV_Pin_Init(void)
{
    //UV_PIN_PxM0 &= ~(1 << UV_PIN_PORTBIT);
    UV_PIN_PxM0 |= (1 << UV_PIN_PORTBIT);
    UV_PIN_PxM1 &= ~(1 << UV_PIN_PORTBIT);
  
    UV_CONTROL_PIN = 0;
  
    ADCInit(d_ADCnEN, d_ADC_CLK_Sel);

}

void UV_On(void)
{
    //unsigned char *wifi_send_buff = "uv on\n";
  
    UV_CONTROL_PIN = 1;
    IsUVOn = 1;
  
    //每次刚上电时，第一次检测到的ADC的值为0，所以初始化为-1，上电后最开始检测4次，忽略第1次，以后每次都是检测3次
    uv_check_times = -1;
  
    //上电后5秒检测ADC的值
    uv_check_timeinterval = UV_CHECK_TIME_INTERVAL - 5;
  
    TM1620_LED_Control(LED_UV,LED_ON);
   
}

void UV_Off(void)
{  
    UV_CONTROL_PIN = 0;
    IsUVOn = 0;
    TM1620_LED_Control(LED_UV,LED_OFF);
  
    ADCfinish = 0;
    IsUVCheck = 0;   
}

void UV_ADC_Start(void)
{
    ADCfinish = 0;
    ADCstart(d_ADC_CH0_IN);
}

unsigned int Get_UV_ADC_Data(void)
{
    unsigned int adc_data = 0;
  
    adc_data = n_data;
    n_data = 0;
    ADCfinish = 0;
    return adc_data;
}

//每次检测3秒钟，每1秒检测一次，3次数据都正常才可以，3次检测中如果有一次检测到的数据不正常则判定为UV出故障
void UV_Check(void)
{
    unsigned int uv_adc_data;
    unsigned char uv_adc_byte[3] = {0};
    //unsigned char wifi_send_buff[50] = {0};
  
    if(IsUVOn == 1)
    {           
        if(IsUVCheck == 1 && ADCfinish == 1)
        {
            uv_adc_data = Get_UV_ADC_Data();
         
            if(uv_adc_data < UV_ADC_CRITICAL_VALUE && uv_check_times >= 0)
            {
                IsUVfault = 1;
            }
            if(++uv_check_times == 3)
            {   
                IsUVCheck = 0;
                uv_check_flag = 0;
                uv_check_times = 0;     
            }

//            mymemset(wifi_send_buff,0,sizeof(wifi_send_buff));
//            sprintf(wifi_send_buff,"adc data: %d,IsUVfault:%d\n",(unsigned int)uv_adc_data,(unsigned int)IsUVfault);
//            Wifi_Uart_Sendbytes(wifi_send_buff,mystrlen(wifi_send_buff));            

            
        }
        
        
        if(IsUVCheck)
        {
            if(uv_check_flag)
            {
                UV_ADC_Start();
                uv_check_flag = 0;
            }
        }
        

        
        if(uv_check_timeinterval >= UV_CHECK_TIME_INTERVAL)
        {
            IsUVCheck = 1;
            uv_check_timeinterval = 0;
        }
        
        
        if(IsUVfault == 1)
        {
            UV_Off();
        }
    }
}