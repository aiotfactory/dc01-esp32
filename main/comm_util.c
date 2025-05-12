#include <stdint.h>
#include <util.h>
#include <spi_util.h>
#include <i2cutil.h>
#include <tm7705_util.h>
#include <camera_util.h>
#include <device_config.h>
#include <gpio_util.h>
#include <uart_util.h>
#include <rs485_util.h>
#include <lora_util.h>
#include <bat_adc.h>
#include <system_util.h>
#include <meta_util.h>
#include <forward_util.h>
#include <cloud_log.h>
#include <aht20.h>
#include <spl06.h>
#include <thermal_util.h>
#include <ultrasonic_util.h>

#define TEMP_OUT_BUF_LEN 2048

static const char *TAG = "comm_util";

	
void response_process(uint32_t command,uint32_t flag,int32_t errorType,uint32_t pack_seq,const uint8_t *data_in,uint32_t data_in_len)
{
	//如果开启云日志，只能用printf，否则会导致循环上报日志
	//printf("response_process command %lu flag %lu errorType %ld pack_seq %lu data len %lu\r\n",command,flag,errorType,pack_seq,data_in_len);
}
data_send_element *operate_process(uint32_t operate,uint8_t *data_in,uint32_t data_in_len)
{
	data_send_element *data_element1 = (data_send_element*)user_malloc(sizeof(data_send_element));
	data_element1->need_free = 1;
	data_element1->tail = NULL; 
	data_element1->data=user_malloc(TEMP_OUT_BUF_LEN);
	data_element1->data_len=-1;
	
	data_send_element *data_element2 = (data_send_element*)user_malloc(sizeof(data_send_element));
    data_element2->tail = NULL; 
    
    data_element1->tail=data_element2;
    
	switch(operate)
	{
		case 1:
			command_operate_spi_init(data_in,data_in_len,data_element1->data,&(data_element1->data_len));
			break;
		case 2:
			command_operate_gpio_ext_set(data_in,data_in_len,data_element1->data);
			break;
		case 3:	
			command_operate_spi_inout(data_in,data_in_len,data_element1->data,&(data_element1->data_len));
			break;
		case 4:	
			spi_property(data_element1->data,&(data_element1->data_len));
			break;
		case 5:	
			tm7705_property(data_element1->data,&(data_element1->data_len));
			break;
		case 6:	
			camera_property(data_element1->data,&(data_element1->data_len));
			break;
		case 7:	
			gpio_util_property(data_element1->data,&(data_element1->data_len));
			break;
		case 8:	
			uart_util_tx(data_in,data_in_len);
			break;
		case 9:
			command_operate_gpio_esp_set(data_in,data_in_len,data_element1->data);
			break;
		case 10:	
			lora_util_tx(data_in,data_in_len,data_element1->data,&(data_element1->data_len));
			break;
		case 11:	
			command_operate_i2c_inout(data_in,data_in_len,data_element1->data,&(data_element1->data_len),TEMP_OUT_BUF_LEN);
			break;
		case 12:
			bat_adc_property(data_element1->data,&(data_element1->data_len));
			break;
		case 13:
			meta_property(data_element1->data,&(data_element1->data_len));
			break;					
		case 14:
			tm7705_read(data_in,data_in_len,data_element1->data,&(data_element1->data_len));
			break;		
		case 15:	
			camera_request(data_element2);
			break;		
		case 16:		
			command_operate_gpio_read(data_in,data_in_len,data_element1->data,&(data_element1->data_len));
			break;	
		case 17:		
			forward_property(data_in,data_in_len,data_element2);
			break;	
		case 18:		
			config_request(data_element2);
			break;		
		case 19:		
			aht20_request(data_element1->data,&(data_element1->data_len));
			break;		
		case 20:		
			spl06_request(data_element1->data,&(data_element1->data_len));
			break;	
		case 21:		
			lora_util_property(data_element1->data,&(data_element1->data_len));
			break;		
		case 22:	
			rs485_util_tx(data_in,data_in_len);
			break;			
		case 23:	
			thermal_request(data_element2);
			break;					
		case 24:	
			ultrasonic_request(data_element2);
			break;		
		default:
			ESP_LOGW(TAG, "operate_process recv wrong operate %lu",operate);
			sprintf((char *)data_element1->data,"wrong command %lu",operate);
	}
	return data_element1;
}