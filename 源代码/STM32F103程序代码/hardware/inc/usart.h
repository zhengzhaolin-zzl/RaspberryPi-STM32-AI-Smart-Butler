#ifndef _USART_H_
#define _USART_H_

#include "stm32f10x.h"

#define USART_DEBUG		USART1		
#define USART_Pinmu		USART3	

extern char Serial_RxPacket[];
extern uint8_t Serial_RxFlag;

void Usart1_Init(unsigned int baud);
void Usart2_Init(unsigned int baud);
void Usart3_Init(unsigned int baud);
void Usart3_SendString(char *String);
void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);

#endif
