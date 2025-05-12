#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include <device_config.h>
#include <util.h>
#include <network_util.h>
#include <gpio_util.h>
#include <cloud_log.h>
#include "system_util.h"

//static const char *TAG = "forward_util";

//data will be uploaded regularly [CONFIG_FORWARD_UPLOAD_INTERVAL_SECONDS]
int forward_upload(void *parameter)
{
	static uint32_t times=0;
	int ret=-1;
	uint8_t *temp_buff=user_malloc(64);
	sprintf((char *)temp_buff,"hello, here is the data you can modify %lu.",times);
	int temp_buff_len=strlen((char *)temp_buff);
	ret=tcp_send_command(
	      "forward",
	      data_container_create(1,COMMAND_REQ_FORWARD,temp_buff,temp_buff_len),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	times++;
	return ret;
}
//this func will response to the request from cloud
void forward_property(uint8_t *command_data,uint32_t command_data_len,data_send_element *data_element)
{
	static uint32_t times=0;
	//print what are passed in from request coming from cloud
	cloud_printf_format("forward_property request ", "", "%02x",command_data,command_data_len>100?100:command_data_len);
	
	uint8_t *temp_buff=user_malloc(64);
	sprintf((char *)temp_buff,"hello, response to your request %lu.",times);
	int temp_buff_len=strlen((char *)temp_buff);
	//do you logic
	//...
	data_element->data=temp_buff;
	data_element->data_len=temp_buff_len;
	data_element->need_free=1;
	//ESP_LOGI(TAG, "forward_property %lu",times);
	//don't need to free temp_buff
	
	times++;
}

