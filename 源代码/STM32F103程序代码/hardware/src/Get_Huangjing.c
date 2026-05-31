#include "stm32f10x.h"                  // Device header
#include "AD.h"
#include "dht11.h"
#include "Get_Huangjing.h"
#include "HMI_Rx.h"

uint8_t temperature,humi;
uint8_t temperature_ed,humi_ed;
uint8_t Mq135,Fire;
uint8_t Mq135_ed,Fire_ed;
uint8_t Mq135_value,Fire_value;
uint8_t First_flag;

void Huanjing_Get()
{
	temperature_ed=temperature,humi_ed=humi;
	Mq135_ed=Mq135,Fire_ed=Fire;
	DHT11_Read_Data(&temperature,&humi);
	Huanjing_temphumi("huanjing.t4.txt",temperature);
	delay_ms(100);
	Huanjing_temphumi("huanjing.t5.txt",humi);
	
	Mq135_value=Get_Percentage_value(0);
	Fire_value=Get_Percentage_value(1);
	if(Mq135_value>=75){Mq135=0;}
	else if(Mq135_value<75&&Mq135_value>=55){Mq135=1;}
	else if(Mq135_value<55&&Mq135_value>=0){Mq135=2;}
	if(Fire_value>=35){Fire=1;}
	else{Fire=0;}		
	if(Mq135_ed!=Mq135||First_flag==0)
	{
		if(Mq135==0)
		{
			Huanjing_mq134fire("huanjing.t7.txt","”≈");
		}
		else if(Mq135==1)
		{
			Huanjing_mq134fire("huanjing.t7.txt","¡º");
		}
		else
		{
			Huanjing_mq134fire("huanjing.t7.txt","≤Ó");
		}
	}
	if(Fire_ed!=Fire||First_flag==0)
	{
		if(Fire==0)
		{
			Huanjing_mq134fire("huanjing.t8.txt","Œﬁª‘÷");
		}
		else
		{
			Huanjing_mq134fire("huanjing.t8.txt","”–ª‘÷");
		}		
	}
	First_flag=1;
}


