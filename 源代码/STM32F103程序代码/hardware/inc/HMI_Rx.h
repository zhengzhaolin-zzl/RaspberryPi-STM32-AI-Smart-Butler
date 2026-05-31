#ifndef __HMI_Rx_H
#define __HMI_Rx_H

#include "stm32f10x.h"                  // Device header
#include "delay.h"

typedef struct {
    char age[10];     // 年龄
    char xinbie[10];  // 性别
    char hight[10];   // 身高
    char weight[10];  // 体重
	char guoming[20]; // 过敏
	char changwei[20];//肠胃
	char tanniao[20]; //糖尿
	char xuezhi[20]; //高血脂
	char xinxue[20]; //心血管疾病
    char xueya[20];   //血压
} PersonalInfo;


void extractField(const char *data, const char *startField, const char *endField, char *target, uint8_t targetSize);
void storeData(const char *data, PersonalInfo *info);
void peopleData(uint8_t number,PersonalInfo *people);
void healthyData(uint8_t number,PersonalInfo *people);
void Huanjing_temphumi(char* pingmu,uint8_t value);
void Huanjing_mq134fire(char* pingmu,char* str);
void Question(char* input,char* prefix);
#endif
