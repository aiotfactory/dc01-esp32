/*!
 * \file      sx1262mbxcas-board.c
 *
 * \brief     Target board SX1262MBXCAS shield driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdlib.h>
#include "util.h"
#include "radio.h"
#include "llcc68_board.h"
#include "spi_util.h"
#include "stdio.h"
#include "esp_timer.h"
#include "device_config.h"
#include "system_util.h"

struct timer_holder {
	esp_timer_handle_t timer;
    uint32_t milliseconds;
    uint8_t status;
    char *name;
};
typedef struct timer_holder timer_holder_t;
#define TimerEvent_t timer_holder_t
static uint8_t timeinited=0;
static DioIrqHandler *dio1IrqCallback=NULL;	//记录DIO1的回调函数句柄
static TimerEvent_t TxTimeoutTimer;
static TimerEvent_t RxTimeoutTimer;
static const char *TAG = "llcc68_board";

void timer_util_init(char *name,esp_timer_handle_t *timer, void ( *callback1 )( void *) )
{
    const esp_timer_create_args_t oneshot_timer_args = {
            .callback = callback1,
            .name = "one-shot",
			.arg = (void *)name
    };
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, timer));
}
void timer_util_start(esp_timer_handle_t *timer,uint32_t milliseconds)
{
    ESP_ERROR_CHECK(esp_timer_start_once(*timer, milliseconds*1000));
}
void timer_util_stop(esp_timer_handle_t *timer)
{
	esp_timer_stop(*timer);
}
void TimerStop(timer_holder_t *obj)
{
	if(obj->status==1)
	{
		obj->status=0;
		timer_util_stop(&obj->timer);
	}
}
void TimerSetValue(timer_holder_t *obj, uint32_t value )
{
	obj->milliseconds=value;
}
void TimerStart(timer_holder_t *obj)
{
	if(obj->status==1)
	{
		TimerStop(obj);
	}
	obj->status=1;
	timer_util_start(&obj->timer,obj->milliseconds);
}

void TimerInit(char *name1,timer_holder_t *obj,void ( *callback )( void *))
{
	//ESP_LOGI(TAG,"TimerInit %s",name1);
	memset(obj,0,sizeof(timer_holder_t));
	obj->name=name1;
	obj->status=0;
	timer_util_init(name1,&obj->timer,callback);
}
static void IRAM_ATTR lora_dio1_isr_handler(void* arg)
{
    LLCC68IoIrqInit_Handler();
}
void lora_dio1_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL<<LORA_DIO1);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);
    gpio_isr_handler_add(LORA_DIO1, lora_dio1_isr_handler, (void*) LORA_DIO1);
}
void lora_dio1_deinit(void)
{
	gpio_isr_handler_remove(LORA_DIO1);
	util_gpio_init(LORA_DIO1,GPIO_MODE_OUTPUT,GPIO_PULLDOWN_DISABLE,GPIO_PULLUP_DISABLE,0);
}
/* 初始化LLCC68需要用到的GPIO初始化，将 BUSY 引脚设置为输入模式
 */
void LLCC68IoInit( void )
{
	util_gpio_init(LORA_BUSY,GPIO_MODE_INPUT,GPIO_PULLDOWN_DISABLE,GPIO_PULLUP_ENABLE,2);
	spiutil_init(SPI_LORA,"lora",NULL);
}

/* 初始化 DIO1
 * 将DIO1设置为外部中断(上升沿触发)，并且回调函数为 dioIrq 函数原型 void RadioOnDioIrq( void* context )
 */
void LLCC68IoIrqInit( DioIrqHandler dioIrq )
{
	dio1IrqCallback=dioIrq;
	lora_dio1_init();
}
void LLCC68IoIrqInit_Handler(void)
{
	if((NULL!=dio1IrqCallback)&&(gpio_get_level(LORA_DIO1)>0))
		dio1IrqCallback(NULL);
}  
void LLCC68IoDeInit( void )
{
    //GPIO去初始化代码
	lora_dio1_deinit();
	spiutil_deinit(SPI_LORA);
	util_gpio_init(LORA_BUSY,GPIO_MODE_OUTPUT,GPIO_PULLDOWN_DISABLE,GPIO_PULLUP_DISABLE,0);
	if(timeinited)
	{
		timeinited=0;
		TimerStop( &RxTimeoutTimer );
		TimerStop( &TxTimeoutTimer );
	}
}

//复位按键功能
void LLCC68Reset( void )
{
	LLCC68DelayMs( 10 );
	ch423_output(CH423_OC_IO5, 0);
	LLCC68DelayMs( 20 );
	ch423_output(CH423_OC_IO5, 1);
	LLCC68DelayMs( 10 );
}

//读取Busy引脚电平状态
void LLCC68WaitOnBusy( void )
{
	uint32_t u32_count=0;
	while(gpio_get_level(LORA_BUSY)>0){
		if(u32_count++>1000){
			ESP_LOGE(TAG, "LLCC68WaitOnBusy timeout");
			u32_count=0;
		}
		LLCC68DelayMs(1);
	}
}

//设置SPI的NSS引脚电平，0低电平，非零高电平
void LLCC68SetNss(uint8_t lev ){  
	if(lev){
		//输出高电平
		ch423_output(CH423_IO7, 1);
	}else{
		//输出低电平
		ch423_output(CH423_IO7, 0);
	}
}

//spi传输一个字节的数据
uint8_t LLCC68SpiInOut(uint8_t data){
	return spiutil_inout_byte(SPI_LORA,data);
}

//检查频率是否符合要求，如果不需要判断则可以直接返回true
bool LLCC68CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

//毫秒延时
void LLCC68DelayMs(uint32_t ms){
	util_delay_ms(ms);
}

//初始化定时器(这里用TIM3定时器模拟出了两个ms定时器)
//tx定时结束需要回调 RadioOnTxTimeoutIrq
//rx定时结束需要回调 RadioOnRxTimeoutIrq
void LLCC68TimerInit(void){
    if(timeinited==0)
    {
    	timeinited=1;
		TimerInit("TxTimeoutTimer",&TxTimeoutTimer, RadioOnTxTimeoutIrq );
		TimerInit("RxTimeoutTimer",&RxTimeoutTimer, RadioOnRxTimeoutIrq );
    }
}
void LLCC68SetTxTimerValue(uint32_t nMs){	
	TimerSetValue(&TxTimeoutTimer, nMs);
}

void LLCC68TxTimerStart(void){
	TimerStart(&TxTimeoutTimer);
}

void LLCC68TxTimerStop(void){
	TimerStop(&TxTimeoutTimer);
}

void LLCC68SetRxTimerValue(uint32_t nMs){
	TimerSetValue(&RxTimeoutTimer, nMs);
}

void LLCC68RxTimerStart(void){
	TimerStart(&RxTimeoutTimer);
}

void LLCC68RxTimerStop(void){
	TimerStop(&RxTimeoutTimer);
}
