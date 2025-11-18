#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "system_util.h"
#include <device_config.h>
#include <util.h>
#include <network_util.h>
#include <pir_util.h>
#include <camera_util.h>

static uint32_t trigger_times_total=0,trigger_times_from_previous_upload=0,previous_change_seconds=0,state_keep_seconds=0;
static uint8_t pir_status=0,type=0,camera_onoff=0;

static uint32_t config_not_repeat_seconds=0,config_action_times=0,config_action_interval_seconds=0,last_seconds=0;
static uint8_t config_mode=0,config_pin=0,config_valid_times=0,config_action=0;


static const char *TAG = "pir_util";

static void IRAM_ATTR pir_util_isr_handler(void* arg)
{
	trigger_times_total++;
	trigger_times_from_previous_upload++;
	uint32_t temp_seconds=util_get_run_seconds();
	state_keep_seconds=temp_seconds-previous_change_seconds;
	previous_change_seconds=temp_seconds;
	if(trigger_times_from_previous_upload<config_valid_times)
		return;
	if((temp_seconds-last_seconds) < config_not_repeat_seconds)
		return;

	last_seconds = 	temp_seconds;

    uint32_t gpio_num = (uint32_t) arg;
    uint32_t gpio_level=gpio_get_level(gpio_num&0xff);
	gpio_num|=(gpio_level<<31);//bit31=indicate level,bit30=0:gpio,1:pir
    BaseType_t xHigherPriorityTaskWoken;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if( xHigherPriorityTaskWoken )
 	{
       portYIELD_FROM_ISR ();
  	}
}

static void pir_util_isr_set(void)
{
	gpio_config_t io_conf = {};
	
    //GPIO_INTR_DISABLE = 0,     /*!< Disable GPIO interrupt                             */
    //GPIO_INTR_POSEDGE = 1,     /*!< GPIO interrupt type : rising edge                  */
    //GPIO_INTR_NEGEDGE = 2,     /*!< GPIO interrupt type : falling edge                 */
    //GPIO_INTR_ANYEDGE = 3,     /*!< GPIO interrupt type : both rising and falling edge */
    //GPIO_INTR_LOW_LEVEL = 4,   /*!< GPIO interrupt type : input low level trigger      */
    //GPIO_INTR_HIGH_LEVEL = 5,  /*!< GPIO interrupt type : input high level trigger     */
    //GPIO_INTR_MAX,
    

	io_conf.intr_type = config_mode;
	io_conf.pin_bit_mask = (1ULL << config_pin);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en=1;
    gpio_config(&io_conf);
    gpio_isr_handler_add(config_pin, pir_util_isr_handler, (void *)((1 << 30) | (config_mode << 8) | config_pin));
    ESP_LOGI(TAG, "pir_util_isr_set pin %u int_type %u",config_pin,config_mode);
}

int pir_upload(void *parameter)
{
    static uint8_t isr_init=0;
	if(isr_init==0)
	{
		isr_init=1;
		config_mode=device_config_get_number(CONFIG_PIR_MODE);
		if(config_mode==0) return 0;
		power_onoff_spi(1);
		config_valid_times=device_config_get_number(CONFIG_PIR_VALID_TIMES);
		config_not_repeat_seconds=device_config_get_number(CONFIG_PIR_NOT_REPEAT_UPLOAD_SECONDS);
		config_pin=device_config_get_number(CONFIG_PIR_GPIO_NUM);
		config_action=device_config_get_number(CONFIG_PIR_ACTION);
		config_action_times=device_config_get_number(CONFIG_PIR_ACTION_TIMES);
		config_action_interval_seconds=device_config_get_number(CONFIG_PIR_ACTION_INTERVAL_SECONDS);
		camera_onoff=device_config_get_number(CONFIG_CAMERA_ONOFF);
		pir_util_isr_set();
	}
			
	uint8_t *temp_buf=user_malloc(512);
	int temp_buf_idx=0,ret=-1;
	
	if(parameter==NULL)
		pir_status=gpio_get_level(config_pin);
	else
 		pir_status=(((uint32_t )parameter>>31)&0xff);;	
	
	if(type==0)//regular
		state_keep_seconds=util_get_run_seconds()-previous_change_seconds;
	COPY_TO_BUF_WITH_NAME(type,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(trigger_times_total,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(trigger_times_from_previous_upload,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(state_keep_seconds,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(pir_status,temp_buf,temp_buf_idx);
	
	ret=tcp_send_command(
	      "pir",
	      data_container_create(1,COMMAND_REQ_PIR,temp_buf, temp_buf_idx,NULL),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
    
	type=0;
		
	return ret;
}

void pir_util_queue(uint32_t io_num)
{
	uint8_t pin=0,pin_level=0,pin_mode=0;
	pin_level=((io_num>>31)&0xff);
	pin_mode=(io_num>>8)&0xff;
	pin=io_num&0xff;
	
	//ESP_LOGE(TAG, "x1 pin %u level %u mode %u seconds %lu",pin,pin_level,pin_mode,state_keep_seconds);
	
	
	if((pin!=config_pin)||(pin_mode!=config_mode)) return;
	
	util_delay_ms(50);
	if(gpio_get_level(pin)==pin_level)
	{

		pir_status=pin_level;
		type=1;
		
		//ESP_LOGI(TAG, "pir_util_queue pin %u level %u mode %u",pin,pin_level,pin_mode);
		//handle interrupt
		trigger_times_from_previous_upload=0;
		
		pir_upload((void *)io_num);		
		
		if((config_action&0x01)&&(camera_onoff)) // take camera
		{
			for(uint32_t i=0;i<config_action_times;i++)
			{
				printf("camera %lu|%lu\r\n",i,config_action_times);
				if(module_lock("isr",MODULE_CAMERA, portMAX_DELAY) == pdTRUE)
				{
					camera_upload(NULL);
					module_unlock("isr",MODULE_CAMERA);
				}
				util_delay_ms(config_action_interval_seconds*1000);
			}
		}				
    }
}
