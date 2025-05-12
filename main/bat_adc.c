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
#include "system_util.h"

static const char *TAG = "bat_adc";

extern gpio_adc_info adc_pin01_adc1_chan0;

static uint32_t bat_adc_times=0,bat_adc_times_failed=0;
static uint8_t bat_adc_pin_onoff=0;
static int bat_adc_value=0;
static int bat_adc_read(uint8_t *data_out,int *data_idex)
{
	static uint8_t temp_onoff_bak=0;
	uint8_t temp_onoff;
	int ret=-1;
	int temp_idex=0;
	bat_adc_value=0;
	temp_onoff=device_config_get_number(CONFIG_ADC_BAT_ONOFF);
    if(temp_onoff==0)
    {
		if(temp_onoff_bak)//turn off
		{
			temp_onoff_bak=0;
			gpio_util_adc_read(&adc_pin01_adc1_chan0,true);
			power_onoff_bat_adc(0);
			bat_adc_pin_onoff=0;
			ESP_LOGI(TAG, "bat adc turned off");
		}
		ret=1;
		goto ERROR_PROCESS;
	}  
	if(temp_onoff_bak==0)
	{
		power_onoff_bat_adc(1);
		bat_adc_pin_onoff=1;
		ESP_LOGI(TAG, "bat adc turned on");	
		temp_onoff_bak=temp_onoff;
	}
	bat_adc_times++;
	if(gpio_util_adc_read(&adc_pin01_adc1_chan0,false)==0)
	{
		bat_adc_value=adc_pin01_adc1_chan0.value_voltage;
		ret=0;
	}else {
		bat_adc_times_failed++;
	}
ERROR_PROCESS:
	COPY_TO_BUF_WITH_NAME(bat_adc_times,data_out,temp_idex)
	COPY_TO_BUF_WITH_NAME(bat_adc_times_failed,data_out,temp_idex)
	COPY_TO_BUF_WITH_NAME(bat_adc_pin_onoff,data_out,temp_idex)
	COPY_TO_BUF_WITH_NAME(bat_adc_value,data_out,temp_idex)
	*data_idex=temp_idex;
	return ret;
}
int bat_adc_upload(void *parameter)
{
	uint8_t *temp_buff=user_malloc(256);
	int temp_buff_len=0;
	int ret=bat_adc_read(temp_buff,&temp_buff_len);	
	ret=tcp_send_command(
      "bat_adc",
      data_container_create(1,COMMAND_REQ_BAT_ADC,temp_buff,temp_buff_len),
      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	return ret;
}
void bat_adc_property(uint8_t *data_out,int *data_len_out)
{
	bat_adc_read(data_out,data_len_out);
}

