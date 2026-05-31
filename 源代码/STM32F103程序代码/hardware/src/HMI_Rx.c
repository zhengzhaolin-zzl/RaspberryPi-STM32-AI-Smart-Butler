#include <string.h>
#include "HMI_Rx.h"
#include "usart.h"

extern char yinshi[3][128];
extern char healthy[256];
char finalbuffer[64];
extern char ai_question[128];

void extractField(const char *data, const char *startField, const char *endField, char *target, uint8_t targetSize)
{
    const char *start = strstr(data, startField);
    if (!start)
	{
        *target = '\0'; // 如果找不到字段，将目标字符串置空
        return;
    }
    start += strlen(startField); // 跳过字段名
    const char *end;
    if (endField) 
	{
        end = strstr(data, endField);
        if (!end) {
            *target = '\0'; // 如果找不到结束字段，将目标字符串置空
            return;
        }
    } 
	else 
	{
        end = data + strlen(data); // 如果没有结束字段，提取到字符串末尾
    }
    int length = end - start;
    if (length > 0) 
	{
        length = (length < (int)targetSize - 1) ? length : (int)targetSize - 1; // 防止缓冲区溢出
        strncpy(target, start, length);
        target[length] = '\0'; // 确保字符串以 null 结尾
    } 
	else
	{
        *target = '\0'; // 如果长度为0，将目标字符串置空
    }
}


// 存储数据到结构体的函数
void storeData(const char *data, PersonalInfo *info)
{
    // 提取每个字段
    extractField(data, "age", "xinbie", info->age, sizeof(info->age));
    extractField(data, "xinbie", "hight", info->xinbie, sizeof(info->xinbie));
    extractField(data, "hight", "weight", info->hight, sizeof(info->hight));
    extractField(data, "weight", "guoming", info->weight, sizeof(info->weight));
    extractField(data, "guoming", "changwei", info->guoming, sizeof(info->guoming));
    extractField(data, "changwei", "tanniao", info->changwei, sizeof(info->changwei));
    extractField(data, "tanniao", "xuezhi", info->tanniao, sizeof(info->tanniao));
    extractField(data, "xuezhi", "xinxue", info->xuezhi, sizeof(info->xuezhi));
    extractField(data, "xinxue", "xueya", info->xinxue, sizeof(info->xinxue));
    extractField(data, "xueya", NULL, info->xueya, sizeof(info->xueya));
}

void peopleData(uint8_t number,PersonalInfo *people)
{
	sprintf(yinshi[number], "第%d个人%s岁性别%s,身高%scm体重%skg,过敏源%s",number+1,people->age,
	people->xinbie,people->hight,people->weight,people->guoming);
}

void healthyData(uint8_t number,PersonalInfo *people)
{
	uint8_t found_changwei = 0, found_tanniao = 0, found_xuezhi = 0, found_xinxue = 0, found_xueya = 0;
	for(int i=0;i<number;i++)
	{
        if (!found_changwei && strcmp(people[i].changwei, "有肠胃疾病") == 0) 
		{
            strcat(healthy, "有人肠胃疾病");
            found_changwei = 1;
        }
		 if (!found_tanniao && strcmp(people[i].tanniao, "患糖尿病") == 0) 
		{
            strcat(healthy, "有人患糖尿病");
            found_tanniao = 1;
        }
		if (!found_xuezhi && strcmp(people[i].xuezhi, "患高血脂") == 0) 
		{
            strcat(healthy, "有人患高血脂");
            found_xuezhi = 1;
        }
		if (!found_xinxue && strcmp(people[i].xinxue, "患心血管疾病") == 0) 
		{
            strcat(healthy, "有人患心血管疾病");
            found_xinxue = 1;
        }
		if (!found_xueya && strcmp(people[i].xueya, "患高血压") == 0) 
		{
            strcat(healthy, "有人患高血压");
            found_xueya = 1;
        }
	}
}

void Huanjing_temphumi(char* pingmu,uint8_t value)
{
	sprintf(finalbuffer,"%s=\"%d\"\xff\xff\xff",pingmu,value);
	Usart3_SendString(finalbuffer);
	UsartPrintf(USART1, finalbuffer);
}

void Huanjing_mq134fire(char* pingmu,char* str)
{
	sprintf(finalbuffer,"%s=\"%s\"\xff\xff\xff",pingmu,str);
	Usart3_SendString(finalbuffer);
	UsartPrintf(USART1, finalbuffer);
}

void Question(char* input,char* prefix)
{
	char* start = strstr(input, prefix);
    if (start != NULL) {
        // 跳过前缀部分，从后一个字符开始复制
        strncpy(ai_question, start + strlen(prefix), strlen(input) - strlen(prefix));
        ai_question[strlen(input) - strlen(prefix)] = '\0'; // 添加字符串结束符
    } 
}
