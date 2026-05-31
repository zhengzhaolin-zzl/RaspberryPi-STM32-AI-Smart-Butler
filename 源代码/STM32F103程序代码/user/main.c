#include "stm32f10x.h"
#include "esp8266.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "HMI_Rx.h"
#include "Timer.h"
#include "AD.h"
#include "dht11.h"
#include "Get_Huangjing.h"
#include "max30102.h"
#include "read30102.h"
#include <string.h>

PersonalInfo people[10];
uint8_t people_number=0;
char yinshi[3][128]={0};
char question2[1024];
char question3[128]="aa";
char question4[128]="aa";

char healthy[128]={0};
char ai_question[128]={0};
char new_packet[128];
char sendBuffer[256]; // 根据需要调整缓冲区大小

uint8_t cnt1;         //定时器计数环境
uint8_t Huangjing_Flag;//环境监测许可
uint8_t max30102_Flag=1;

void Usart_Pro()
{
	if (strstr(Serial_RxPacket, "number") != NULL)
	{
		UsartPrintf(USART1, "OK");
		max30102_Flag=1;
		Read_30102();
	}	
	if (strstr(Serial_RxPacket, "age") != NULL)			
	{	
		storeData(Serial_RxPacket,&people[people_number]);
		peopleData(people_number,&people[people_number]);
		people_number++;	
	}
	if (strstr(Serial_RxPacket, "shansishuaxin") != NULL)	
	{
		sprintf(question2, "aa一家人有%d个人，",people_number);
		for (int i = 0; i < people_number; i++) 
		{
			strcat(question2, yinshi[i]);
		}
		healthyData(people_number,people);
		strcat(question2, healthy);
		strcat(question2, "帮我制定今日这个家庭的饮食计划，用400个字符回答");
		Aliyun_Service_yinshi(question2,"shansi.t1.txt");
		strcpy(question2, "");
	}
	if (strstr(Serial_RxPacket, "yundong1shuaxin") != NULL)	
	{
		strcat(question3, yinshi[0]);
		strcat(question3, "帮我制定一下今日的运动计划，用400个字符回答");
		Aliyun_Service(question3,"yundong_xili1.t1.txt");
		strcpy(question3, "aa");
	}
	if (strstr(Serial_RxPacket, "yundong2shuaxin") != NULL)	
	{
		strcat(question3, yinshi[1]);
		strcat(question3, "帮我制定一下今日的运动计划，用400个字符回答");
		Aliyun_Service(question3,"yundong_xili2.t1.txt");
		strcpy(question3, "aa");
	}
	if (strstr(Serial_RxPacket, "yundong3shuaxin") != NULL)	
	{
		strcat(question3, yinshi[2]);
		strcat(question3, "帮我制定一下今日的运动计划，用400个字符回答");
		Aliyun_Service(question3,"yundong_xili3.t1.txt");
		strcpy(question3, "aa");
	}			
	if (strstr(Serial_RxPacket, "question") != NULL)	
	{
		Question(Serial_RxPacket,"question");
		strcat(question4,ai_question);
		strcat(question4,",帮我解答一下这个问题，用300个字符回答");
		Aliyun_Service(question4,"ai_wenda.t3.txt");
		strcpy(question4, "aa");
	}
}

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//中断控制器分组设置
	Delay_Init();	
	Usart1_Init(115200);							//串口1，打印信息用
	Usart2_Init(115200);							//串口2，驱动ESP8266用
	Usart3_Init(9600);                             	//串口屏
	MAX30102_Init();
	UsartPrintf(USART1, "OK");
	ESP8266_Init();					//初始化ESP8266
	
	AD_Init();
	while(DHT11_Init())
	{
		delay_ms(200);
	}
	
	Timer_Init();

	while (1)
	{
		if (Serial_RxFlag == 1)		//如果接收到数据包
		{
			Huangjing_Flag=1;
			Usart_Pro();
			Serial_RxFlag = 0;	//处理完成后，需要将接收数据包标志位清零，否则将无法接收后续数据包
			Huangjing_Flag=0;
		}
		
		if(cnt1%4==0 && Huangjing_Flag==0)
		{
			cnt1=0;
			cnt1++;
			Huanjing_Get();
		}
	}
		
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		cnt1++;
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
