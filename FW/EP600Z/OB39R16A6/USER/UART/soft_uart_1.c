#include "OB39R16A6.h"
#include "soft_uart.h"
#include "global.h"
#include "wifi_uart.h"
#include "debug_uart.h"
//#include "TM1618.h"
/*
//时钟周期 22.1184MHz
1秒的机器周期为 22118400/12


1ms的需要的机器周期为 22118.4/12
22118.4/12 = 1843.2(0x0733)
65536 - 1843 = 43418（0xF8CD）

0.5ms需要的机器周期为 22118.4/12/2 = 921.6
65536 - 921  = 64615(0xFC67)

0.25ms需要的机器周期为 22118.4/12/2/2 = 460.8
65536 - 461  = 65075(0xFE33)

0.1ms需要的机器周期为 22118.4/12/10 = 184.32
65536 - 184  = 65352(0xFF48)

0.05ms = 50us 需要的机器周期为 22118.4/12/10/2 = 92.16
65536 - 92  = 65444(0xFFA4)

104.17us = 0.10417ms 需要的机器周期为 (22118.4/12)/(1/0.10417) = 192 对应的波特率 9600
TMIER1采用模式2,8bit- autoreload模式 TH1 = 256 - 192 = 64 

34.7us = 0.0347ms 需要的机器周期为 (22118.4/12)/(1/0.0347) = 63.96 = 64  对应的波特率为 9600*3
TMIER1采用模式2,8bit- autoreload模式 TH1 = 256 - 64 = 192 
*/

//定时器为104.17us时 U2RxRate_bk = 1；定时器为34.7s时 U2RxRate_bk = 3；
static unsigned char U2RxRate_bk = 0;
static unsigned char U2TxRate_bk = 0;
static unsigned char U3RxRate_bk = 0;
static unsigned char U3TxRate_bk = 0;

#define d_T0MOD    0x20
#define d_T0_TH0   192

void Timer0_Init(void)
{
    ET0 = 1;
    TMOD  |= d_T0MOD;
    TH0   = d_T0_TH0;  //MODE1 16bit
    //TL0   = d_T0_TL0;
    EA   = 1;
    TR0  = 1;
}

void Soft_UART_Init(void)
{
    Timer0_Init();
  
#ifdef WIFI_SOFT_UART
    U2RxRate_bk = 3;
    U2TxRate_bk = 3;
#endif
  
    U3RxRate_bk = 3;
    U3TxRate_bk = 3;
}





/*********************
High priority interupt
TIMER1	34.7us timer, 9600*3 bps
中断函数内部勿使用耗时的函数，否则模拟UART的发送和接收比特率会出问题
******************/
void TIMER0_ISR(void) interrupt d_T0_Vector
{	
#ifdef WIFI_SOFT_UART
    static unsigned char U2RxBitCount = 0;
    static unsigned char U2RxBuf = 0;
    static unsigned char U2TxBuf = 0;
    static unsigned char U2RxRate = 0;
#endif
  
    static unsigned char U3RxBitCount = 0;
    static unsigned char U3RxBuf = 0;
    static unsigned char U3TxBuf = 0;
    static unsigned char U3RxRate = 0;

 /*模拟 UART2函数*/   
#ifdef WIFI_SOFT_UART 
	// Rx
  if(U2RxPtr < U2RxBuff_MAXSIZE)
  {
    if(U2RxBitCount)
    {
      if(!--U2RxRate)
      {
        //用两个判断 U2RxBitCount==9,此处是延时一个停止位
        if(U2RxBitCount == 9)
        {
          U2RxBitCount = 0;		//stop Rxing
          U2RxBuffer[U2RxPtr++] = U2RxBuf;
          U2RxBuf = 0;
          //U2RxRate = U2RxRate_bk;
          IsU2RxBusy = 0;
          
        }
        else
        {
          U2RxRate = U2RxRate_bk;
          if(U2RxPin)
          {
              U2RxBuf |= (1 << 7);
          }
          else
          { 
              U2RxBuf &= ~(1 << 7);
          }
          
          U2RxBitCount++;
          if(U2RxBitCount == 9)
          {
              U2RxRate = U2RxRate_bk;
              return;
          }
          U2RxBuf >>= 1;          
        }
      }
    }
    else
    {
      if(!U2RxPin)
      {
        U2RxBitCount = 1;		//start Rx
        U2RxRate = U2RxRate_bk + 1;
        IsU2RxBusy = 1;
      }
    }  
  }

  
  
	//to initiate a Tx, just write the U8 data to U2TxBuf,and set U2TxBitCount = 10 ,U2TxRate set to 3
	//first check if it's Txing (if U2TxBitCount != 0), 

  
	if(U2TxBitCount){
		if(!--U2TxRate){
			U2TxRate = U2TxRate_bk;
			//
			switch(U2TxBitCount --){
				case 1:		//Stop bit; send next byte
					U2TxPin = 1;
					if(U2TxPtr != U2LdPtr){
						U2TxBuf = U2TxBuffer[U2TxPtr ++];
						U2TxBitCount = 10;
					}
					else{
						U2TxPtr = 0;
						U2LdPtr = 0;
            IsU2TxBusy = 0;
					}
				break;

				case 10:	//Start bit
          IsU2TxBusy = 1;
					U2TxPin = 0;
          if(U2TxPtr == 0)
          {
            U2TxBuf = U2TxBuffer[U2TxPtr ++];
          }
				break;
				
				default:
					//U2TxPin = U2TxBuf & 0XFE;
          U2TxPin = U2TxBuf & 0x01;
					U2TxBuf >>= 1;
				break;

			}
		}
	} 
 #endif
  

/*模拟 UART3函数*/  
	// Rx
  if(U3RxPtr < U3RxBuff_MAXSIZE)
  {
    if(U3RxBitCount)
    {
      if(!--U3RxRate)
      {
        //用两个判断 U2RxBitCount==9,此处是延时一个停止位
        if(U3RxBitCount == 9)
        {
          U3RxBitCount = 0;		//stop Rxing
          U3RxBuffer[U3RxPtr++] = U3RxBuf;
          U3RxBuf = 0;
          IsU3RxBusy = 0;
          
        }
        else
        {
          U3RxRate = U3RxRate_bk;
          if(U3RxPin)
          {
              U3RxBuf |= (1 << 7);
          }
          else
          { 
              U3RxBuf &= ~(1 << 7);
          }
          
          U3RxBitCount++;
          if(U3RxBitCount == 9)
          {
              return;
          }
          U3RxBuf >>= 1;          
        }
      }
    }
    else
    {
      if(!U3RxPin)
      {
        U3RxBitCount = 1;		//start Rx
        U3RxRate = U3RxRate_bk + 1;
        IsU3RxBusy = 1;
      }
    }  
  }

  
 
  
	//to initiate a Tx, just write the U8 data to U2TxBuf,and set U2TxBitCount = 10 ,U2TxRate set to 3
	//first check if it's Txing (if U2TxBitCount != 0), 

  
	if(U3TxBitCount){
		if(!--U3TxRate){
			U3TxRate = U3TxRate_bk;
			//
			switch(U3TxBitCount --){
				case 1:		//Stop bit; send next byte
					U3TxPin = 1;
					if(U3TxPtr != U3LdPtr){
						U3TxBuf = U3TxBuffer[U3TxPtr ++];
						U3TxBitCount = 10;
					}
					else{
						U3TxPtr = 0;
						U3LdPtr = 0;
            IsU3TxBusy = 0;
					}
				break;

				case 10:	//Start bit
          IsU3TxBusy = 1;
					U3TxPin = 0;
          if(U3TxPtr == 0)
          {
            U3TxBuf = U3TxBuffer[U3TxPtr ++];
          }
				break;
				
				default:
          U3TxPin = U3TxBuf & 0x01;
					U3TxBuf >>= 1;
				break;

			}
		}
	}   
	
	
}