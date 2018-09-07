#include "stdio.h"
#include "stm32f10x.h"
#include "user_usart.h"

void Usart_SendOneByte( USART_TypeDef * pUSARTx, uint8_t ch) 
{
    USART_SendData(pUSARTx,ch); 
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET); 
}
 
 
void Usart_SendBytes( USART_TypeDef * pUSARTx,uint8_t *buff,uint8_t length) 
{
    if(buff == NULL || length <= 0)
    {
        return;
    }
    while(length--)
    {
        Usart_SendOneByte(pUSARTx,*buff++);
    }
  
}
 

