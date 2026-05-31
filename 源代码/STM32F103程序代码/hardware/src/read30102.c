#include "delay.h" 
#include "string.h"
#include "read30102.h"
#include "max30102.h"
#include "usart.h"
#include <stdio.h>

#define MAX_BRIGHTNESS 255
#define INTERRUPT_REG 0X00
char buffer1[50];
char buffer2[50];
char buffer3[50];

/* 	VCC<->3.3V
	GND<->GND
	SCL<->PB7
	SDA<->PB8
	INT<->PB9*/

uint32_t aun_ir_buffer[500]; 	 //IR LED   红外光数据，用于计算血氧
int32_t n_ir_buffer_length;    //数据长度
uint32_t aun_red_buffer[500];  //Red LED	红光数据，用于计算心率曲线以及计算心率
int32_t n_sp02; //SPO2值
int8_t ch_spo2_valid;   //用于显示SP02计算是否有效的指示符
int32_t n_heart_rate;   //心率值
int8_t  ch_hr_valid;    //用于显示心率计算是否有效的指示符

uint8_t Temp;

uint32_t un_min, un_max, un_prev_data;  
int i;
int32_t n_brightness;
float f_temp;
//u8 temp_num=0;
u8 temp[6];
u8 str[100];
u8 dis_hr=0,dis_spo2=0;

extern uint8_t max30102_Flag;

// 新增宏定义：最大重试次数（可自定义调整）
#define MAX_RETRIES 20

void Read_30102()
{
    static int retry_count = 0;  // 静态变量记录重试次数
    un_min = 0x3FFFF;
    un_max = 0;
    n_ir_buffer_length = 500;    // 缓冲区长度

    // 读取前500个样本，并确定信号范围
    for (i = 0; i < n_ir_buffer_length; i++)
    {
        while (MAX30102_INT == 1); // 等待中断引脚（需确保此处不会永久阻塞）
        
        max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
        aun_red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2];
        aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5];
        
        if (un_min > aun_red_buffer[i]) un_min = aun_red_buffer[i];
        if (un_max < aun_red_buffer[i]) un_max = aun_red_buffer[i];
    }
    un_prev_data = aun_red_buffer[i];

    // 初始计算心率和血氧
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, 
                                         &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 

    // 修改循环条件：检查重试次数
    while (max30102_Flag == 1 && retry_count < MAX_RETRIES)
    {
        // 移动缓冲区数据（100~500 → 0~400）
        for (i = 100; i < 500; i++)
        {
            aun_red_buffer[i - 100] = aun_red_buffer[i];
            aun_ir_buffer[i - 100] = aun_ir_buffer[i];
            
            // 更新信号范围
            if (un_min > aun_red_buffer[i]) un_min = aun_red_buffer[i];
            if (un_max < aun_red_buffer[i]) un_max = aun_red_buffer[i];
        }

        // 读取新数据（填充400~500）
        for (i = 400; i < 500; i++)
        {
            un_prev_data = aun_red_buffer[i - 1];
            while (MAX30102_INT == 1); // 等待中断引脚
            
            max30102_FIFO_ReadBytes(REG_FIFO_DATA, temp);
            aun_red_buffer[i] = (long)((long)((long)temp[0] & 0x03) << 16) | (long)temp[1] << 8 | (long)temp[2];
            aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03) << 16) | (long)temp[4] << 8 | (long)temp[5];
            
            // 计算亮度（原有逻辑）
            if (aun_red_buffer[i] > un_prev_data) {
                f_temp = (aun_red_buffer[i] - un_prev_data) / (un_max - un_min) * MAX_BRIGHTNESS;
                n_brightness -= (int)f_temp;
                if (n_brightness < 0) n_brightness = 0;
            } else {
                f_temp = (un_prev_data - aun_red_buffer[i]) / (un_max - un_min) * MAX_BRIGHTNESS;
                n_brightness += (int)f_temp;
                if (n_brightness > MAX_BRIGHTNESS) n_brightness = MAX_BRIGHTNESS;
            }
        }

        // 重新计算心率和血氧
        maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer,
                                             &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);

        // 判断数据有效性
        if (ch_hr_valid == 1 && ch_spo2_valid == 1) 
        {
			dis_hr = n_heart_rate;
			dis_spo2 = n_sp02;
			sprintf(buffer1, "%s.n0.val=%d\xff\xff\xff", Serial_RxPacket, dis_hr);
			sprintf(buffer2, "%s.n1.val=%d\xff\xff\xff", Serial_RxPacket, dis_spo2);
			
			Usart3_SendString(buffer1);
			delay_ms(200);
			Usart3_SendString(buffer2);
			delay_ms(200);
			memset(Serial_RxPacket, 0, 1024);
			max30102_Flag = 0;    // 清除标志位
			retry_count = 0;     // 重置重试计数器
			break;                // 退出循环
        }
    }
}

