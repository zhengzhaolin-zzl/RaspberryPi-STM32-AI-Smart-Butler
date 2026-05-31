#include "stm32f10x.h"
#include "esp8266.h"
#include "delay.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>


#define ESP8266_WIFI_INFO		"AT+CWJAP=\"www\",\"www123456\"\r\n"          
#define ALIYUN_INFO             "AT+CIPSTART=\"TCP\",\"172.20.10.2\",8080\r\n"


unsigned char esp8266_buf[1024];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;
char buf[512];
char *question1 = "aa获取今天广州白云区天气预报，从穿搭、外出方面考虑，300个字符回答";

void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		esp8266_cnt = 0;							//清0接收计数
			
		return REV_OK;								//返回接收完成标志
	}
		
	esp8266_cntPre = esp8266_cnt;					//置为相同
	
	return REV_WAIT;								//返回接收未完成标志

}

_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 200;

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存
				
				return 0;
			}
		}
		
		delay_ms(10);
	}
	
	return 1;

}

_Bool ESP8266_SendCmd1(char *cmd, char *res)
{
	
	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(1)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				//ESP8266_Clear();									//清空缓存
				
				return 0;
			}
		}
		
		delay_ms(10);
	}
	
	return 1;

}

void printContentToScreen(char* input,char* yemain) 
{
	char buffer[1024];
	char finalbuffer[1024];
    char* startPos = strstr(input, ":");  // 找到冒号位置
    startPos++;  // 跳过冒号
	char* endPos = strstr(startPos, "this is zzl");
	
	int length = endPos - startPos;  // 计算从冒号
	strncpy(buffer, startPos, length); 
	buffer[length] = '\0'; 
	
	sprintf(finalbuffer,"%s=\"%s\"\xff\xff\xff",yemain,buffer);
	
	Usart3_SendString(finalbuffer);
}

void Aliyun_Service(char *question,char* yemian)
{	
	sprintf(buf,"AT+CIPSEND=%d\r\n",strlen(question));
	
	while(ESP8266_SendCmd(buf, "OK"))
		delay_ms(500);	

	while(ESP8266_SendCmd1(question, "zzl"))
		delay_ms(2000);	
	
	delay_ms(500);
	delay_ms(500);
	delay_ms(500);		
	
	printContentToScreen(esp8266_buf,yemian);	
	
	ESP8266_Clear();
}


void Aliyun_Service_yinshi(char *question,char* yemian)
{	
	sprintf(buf,"AT+CIPSEND=%d\r\n",strlen(question));
	
	while(ESP8266_SendCmd(buf, "OK"))
		delay_ms(500);	
	
	while(ESP8266_SendCmd1(question, "zzl"))
		delay_ms(2000);	
	
	delay_ms(500);
	delay_ms(500);
	delay_ms(500);		
		
	printContentToScreen(esp8266_buf,yemian);	
	
	ESP8266_Clear();
}

void ESP8266_Init(void)
{
	
	GPIO_InitTypeDef GPIO_Initure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//ESP8266复位引脚
	GPIO_Initure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Initure.GPIO_Pin = GPIO_Pin_0;					//GPIOC14-复位
	GPIO_Initure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Initure);
	
	GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
	delay_ms(250);
	GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
	delay_ms(500);
	
	ESP8266_Clear();
	
	while(ESP8266_SendCmd("AT\r\n", "OK"))
		delay_ms(500);
	
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		delay_ms(500);
	
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		delay_ms(500);
	
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
		delay_ms(500);
	
	while(ESP8266_SendCmd(ALIYUN_INFO, "OK"))
		delay_ms(800);	
	
	Aliyun_Service(question1,"tianqiyubao.t1.txt");
}

void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //接收中断
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //防止串口被刷爆
		esp8266_buf[esp8266_cnt++] = USART2->DR;
		
		USART_ClearFlag(USART2, USART_FLAG_RXNE);
	}

}
