#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include <sdkconfig.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_event_base.h>
#include "ch423.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define DDL_ZERO_STRUCT(x)          ddl_memclr((uint8_t*)&(x), (uint32_t)(sizeof(x)))
#define ZYC_CHECK(tag,a, str, goto_tag, ...)                                          \
    do                                                                                \
    {                                                                                 \
        if (!(a))                                                                     \
        {                                                                             \
            ESP_LOGE(tag, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__);     \
            goto goto_tag;                                                            \
        }                                                                             \
    } while (0)
    
#define COPY_TO_VARIABLE_NO_CHECK(variable,data,idex) \
    	memcpy(&(variable),data+idex,sizeof(variable)); \
		idex+=sizeof(variable); \
	    
#define COPY_TO_VARIABLE(variable,data,idex,len) \
    if((idex+sizeof(variable))<=len){ \
    	memcpy(&(variable),data+idex,sizeof(variable)); \
		idex+=sizeof(variable); \
	}
#define COPY_TO_BUF_NO_CHECK(variable,data,idex)\
	memcpy(data+idex,&(variable),sizeof(variable));\
	idex+=sizeof(variable);
#define COPY_TO_BUF(variable,data,idex,len)\
	if((idex+sizeof(variable))<=len){\
		memcpy(data+idex,&(variable),sizeof(variable));\
		idex+=sizeof(variable);\
	}
//1 byte name len, name, 1 byte flag, 2 bytes value len, value
#define COPY_TO_BUF_WITH_NAME(variable,data,idex)\
		data[idex++]=strlen(#variable);\
		sprintf((char *)data+idex,"%s",#variable);\
		idex+=strlen(#variable);\
		data[idex++]=0;\
		data[idex++]=sizeof(variable)%256;\
		data[idex++]=sizeof(variable)/256;\
		memcpy(data+idex,&(variable),sizeof(variable));\
		idex+=sizeof(variable);
		



void ddl_memclr(void *pu8Address, uint32_t u32Count);
void util_pause(char *str);
void power_onoff_i2c(uint8_t status);
uint8_t power_status_i2c(void);
void power_onoff_eth(uint8_t status);
void power_onoff_4g(uint8_t status);
void power_onoff_camera(uint8_t status);
void power_onoff_lora(uint8_t status);
void power_onoff_output_join(uint8_t status);
void power_onoff_output_bat(uint8_t status);
void power_onoff_output_vcc(uint8_t status);
void power_onoff_spi(uint8_t status);
void power_onoff_led(uint8_t status);
uint8_t power_status_spi(void);
void power_onoff_bat_adc(uint8_t status);
void util_reset_4g(int onoff);
void timer_util_init(char *name,esp_timer_handle_t *timer, void ( *callback1 )( void *) );
void timer_util_stop(esp_timer_handle_t *timer);
void timer_util_start(esp_timer_handle_t *timer,uint32_t milliseconds);
void util_gpio_init(uint8_t pin,gpio_mode_t mode,gpio_pulldown_t pulldownmode,gpio_pullup_t pullupmode,uint8_t value);
uint32_t util_get_timestamp(void);


uint8_t util_crc_calc(uint8_t *data, uint8_t data_num,uint32_t polynomial);
void print_mem_info(void);
int print_format(char *dst,uint32_t dst_len,char *format,char *prefix, char *suffix,uint8_t *data,uint32_t data_len);
void print_chip_info(void);
void property_value_add(uint8_t *data_out,int *data_out_len,char *property_name,uint8_t property_name_len,uint8_t property_flag,uint16_t property_value_len,uint8_t *property_value);
uint8_t *byte_search(uint8_t *data, uint32_t data_len,uint8_t target,uint32_t index);
void mem_check(char *prefix);
void led_flash(uint32_t time_ms);

extern QueueHandle_t gpio_evt_queue;
extern EventGroupHandle_t app_evt_group_hdl;

#define MODULE_4G_READY_BIT BIT0
#define MODULE_ETH_READY_BIT BIT1


#ifdef __cplusplus
}
#endif
