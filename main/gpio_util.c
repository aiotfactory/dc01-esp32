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
#include <gpio_util.h>


typedef struct {
    uint8_t is_esp;
    uint8_t pin;
    uint16_t pin_config;
    uint8_t previous_pin_mode;
    gpio_adc_info *adc_info;
} gpio_pin_info;


static gpio_adc_info adc_pin2_adc1_chan1={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_1,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin4_adc1_chan3={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_3,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin5_adc1_chan4={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_4,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin6_adc1_chan5={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_5,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin7_adc1_chan6={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_6,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin15_adc2_chan4={.unit=ADC_UNIT_2,.channel=ADC_CHANNEL_4,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin16_adc2_chan5={.unit=ADC_UNIT_2,.channel=ADC_CHANNEL_5,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin17_adc2_chan6={.unit=ADC_UNIT_2,.channel=ADC_CHANNEL_6,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin18_adc2_chan7={.unit=ADC_UNIT_2,.channel=ADC_CHANNEL_7,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin8_adc1_chan7={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_7,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin3_adc1_chan2={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_2,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin9_adc1_chan8={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_8,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin10_adc1_chan9={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_9,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin11_adc2_chan0={.unit=ADC_UNIT_2,.channel=ADC_CHANNEL_0,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin14_adc2_chan3={.unit=ADC_UNIT_2,.channel=ADC_CHANNEL_3,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};
static gpio_adc_info adc_pin1_adc1_chan0={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_0,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};

gpio_adc_info adc_pin01_adc1_chan0={.unit=ADC_UNIT_1,.channel=ADC_CHANNEL_0,.init=0,.cali_handle=NULL,.oneshot_handle=NULL,.value_raw=-1,.value_voltage=-1,.atten=ADC_ATTEN_DB_12};


static gpio_pin_info gpio_pins[40] = {
		{1, 4, CONFIG_GPIO_ESP_4, 0,&adc_pin4_adc1_chan3}, 
		{1, 5, CONFIG_GPIO_ESP_5, 0,&adc_pin5_adc1_chan4},
		{1, 6, CONFIG_GPIO_ESP_6, 0,&adc_pin6_adc1_chan5},
		{1, 7, CONFIG_GPIO_ESP_7, 0,&adc_pin7_adc1_chan6},
		{1, 15, CONFIG_GPIO_ESP_15, 0,&adc_pin15_adc2_chan4},
		{1, 16, CONFIG_GPIO_ESP_16, 0,&adc_pin16_adc2_chan5},
		{1, 17, CONFIG_GPIO_ESP_17, 0,&adc_pin17_adc2_chan6},
		{1, 18, CONFIG_GPIO_ESP_18, 0,&adc_pin18_adc2_chan7},
		{1, 8, CONFIG_GPIO_ESP_8, 0,&adc_pin8_adc1_chan7},
		{1, 3, CONFIG_GPIO_ESP_3, 0,&adc_pin3_adc1_chan2},
		{1, 46, CONFIG_GPIO_ESP_46, 0,NULL},
		{1, 9, CONFIG_GPIO_ESP_9, 0,&adc_pin9_adc1_chan8},
		{1, 10, CONFIG_GPIO_ESP_10, 0,&adc_pin10_adc1_chan9},
		{1, 11, CONFIG_GPIO_ESP_11, 0,&adc_pin11_adc2_chan0},
		{1, 14, CONFIG_GPIO_ESP_14, 0,&adc_pin14_adc2_chan3},
		{1, 21, CONFIG_GPIO_ESP_21, 0,NULL},
		{1, 48, CONFIG_GPIO_ESP_48, 0,NULL},
		{1, 45, CONFIG_GPIO_ESP_45, 0,NULL},
		{1, 0, CONFIG_GPIO_ESP_0, 0,NULL},
		{1, 38, CONFIG_GPIO_ESP_38, 0,NULL},
		{1, 39, CONFIG_GPIO_ESP_39, 0,NULL},
		{1, 40, CONFIG_GPIO_ESP_40, 0,NULL},
		{1, 41, CONFIG_GPIO_ESP_41, 0,NULL},
		{1, 42, CONFIG_GPIO_ESP_42, 0,NULL},
		{1, 2, CONFIG_GPIO_ESP_2, 0,&adc_pin2_adc1_chan1},
		{1, 1, CONFIG_GPIO_ESP_1, 0,&adc_pin1_adc1_chan0},
		{0, 7, CONFIG_GPIO_EXT_IO7, 0,NULL},
		{0, 9, CONFIG_GPIO_EXT_OC1, 0,NULL},
		{0, 13, CONFIG_GPIO_EXT_OC5, 0,NULL},
		{0, 14, CONFIG_GPIO_EXT_OC6, 0,NULL},
		{0, 17, CONFIG_GPIO_EXT_OC9, 0,NULL},
		{0, 18, CONFIG_GPIO_EXT_OC10, 0,NULL},
		{0, 19, CONFIG_GPIO_EXT_OC11, 0,NULL},
		{0, 20, CONFIG_GPIO_EXT_OC12, 0,NULL},
		{0, 21, CONFIG_GPIO_EXT_OC13, 0,NULL},
		{0, 22, CONFIG_GPIO_EXT_OC14, 0,NULL},
		{0, 0, CONFIG_GPIO_EXT_IO0, 0,NULL},
		{0, 1, CONFIG_GPIO_EXT_IO1, 0,NULL},
		{0, 2, CONFIG_GPIO_EXT_IO2, 0,NULL},
		{0, 3, CONFIG_GPIO_EXT_IO3, 0,NULL}
};
       

static const char *TAG = "gpio_util";


int gpio_util_adc_read(gpio_adc_info *adc_info,bool tear_down)
{
	if(adc_info->init==0)//start to init
	{ 
	    adc_oneshot_unit_init_cfg_t init_config = {
	        .unit_id = adc_info->unit,
	    };
   		ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_info->oneshot_handle));
	    adc_oneshot_chan_cfg_t config = {
	        .bitwidth = ADC_BITWIDTH_DEFAULT,
	        .atten = adc_info->atten,
	    };
    	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_info->oneshot_handle, adc_info->channel, &config));
    	
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = adc_info->unit,
            .chan = adc_info->channel,
            .atten = adc_info->atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        if(adc_cali_create_scheme_curve_fitting(&cali_config, &adc_info->cali_handle)==ESP_OK)
        {
			adc_info->cali_result=true;
		}
   		adc_info->init=1;
    }
    if((tear_down)&&(adc_info->init==1))
	{
		ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_info->oneshot_handle));
	    if (adc_info->cali_result) {
	    	//ESP_LOGI(TAG, "gpio_util_adc_read deregister calibration scheme curve fitting");
        	ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(adc_info->cali_handle));
	    }
	    adc_info->cali_result=false;
	    adc_info->init=0;
	    return 0;
	}
    if(adc_info->init==1)
    {
    	ESP_ERROR_CHECK(adc_oneshot_read(adc_info->oneshot_handle, adc_info->channel, &adc_info->value_raw));
    	//ESP_LOGI(TAG, "gpio_util_adc_read adc%d channel %d raw data %d", adc_info->unit + 1, adc_info->channel, adc_info->value_raw);
	    if (adc_info->cali_result) {
	        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_info->cali_handle, adc_info->value_raw, &adc_info->value_voltage));
	        //ESP_LOGI(TAG, "gpio_util_adc_read adc%d channel %d cali voltage %d mv", adc_info->unit + 1, adc_info->channel, adc_info->value_voltage);
	        return 0;
	    }
	}
	return -1;
}

static void IRAM_ATTR gpio_util_isr_handler(void* arg)
{
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

static void gpio_util_isr_set(uint32_t pin_config,uint32_t pin,uint32_t pin_mode,gpio_int_type_t int_type)
{
	gpio_config_t io_conf = {};
	io_conf.intr_type = int_type;
	io_conf.pin_bit_mask = (1ULL<<pin);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en=0;
	if((pin_mode%10)==1)
		io_conf.pull_down_en=1;
	else if((pin_mode%10)==2)
 		io_conf.pull_up_en =1;
    gpio_config(&io_conf);
    
    if(int_type>0)
    	gpio_isr_handler_add(pin, gpio_util_isr_handler, (void*) ((pin_config<<16)|(pin_mode<<8)|pin));
    else
        gpio_isr_handler_remove(pin);
    ESP_LOGI(TAG, "gpio_util_isr_set pin %lu int_type %u mode %lu",pin,int_type,pin_mode);
    //gpio_isr_handler_remove(GPIO_INPUT_IO_0);
}
void gpio_util_interrupt_upload(uint32_t gpio_value)
{
	uint8_t *temp_buf=user_malloc(5);
	temp_buf[0]=1;
	memcpy(temp_buf+1,&gpio_value,4);
	tcp_send_command(
	      "gpio_isr",
	      data_container_create(1,COMMAND_REQ_GPIO,temp_buf, 5,NULL),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
}
static uint32_t out_pin_value_esp=0,out_pin_value_ext=0;
//1: out low 2:out high 30(ADC_ATTEN_DB_0),31(ADC_ATTEN_DB_2_5),32(ADC_ATTEN_DB_6),33(ADC_ATTEN_DB_12): in adc, 40(float),41(pull down),42(pull up): in interrupt high, 5: in interrupt low, 6: in interrupt both, 7: no interrupt
static int gpio_util_process(uint8_t *data_out,int *data_len_out,uint8_t is_esp,uint8_t gpio_no)
{
	static uint8_t temp_onoff_bak=0;
	uint8_t temp_onoff;
	temp_onoff=device_config_get_number(CONFIG_GPIO_ONOFF);
	data_out[0]=2;//no data
	*data_len_out=1;
    if(temp_onoff==0)
    {
		if(temp_onoff_bak)//turn off
		{
			temp_onoff_bak=0;
			for(uint8_t i=0;i<sizeof(gpio_pins) / sizeof(gpio_pins[0]);i++)
			{
				if(gpio_pins[i].is_esp)
				{
					//clear adc if changed
					if((gpio_pins[i].adc_info!=NULL)&&(gpio_pins[i].previous_pin_mode==3))
						gpio_util_adc_read(gpio_pins[i].adc_info,true);
					if(device_config_get_number(gpio_pins[i].pin_config)>0)
						util_gpio_init(gpio_pins[i].pin,GPIO_MODE_OUTPUT,GPIO_PULLDOWN_DISABLE,GPIO_PULLUP_DISABLE,0);
				}
				else
				    ch423_output(gpio_pins[i].pin, 0);
			}
			ESP_LOGI(TAG, "gpio turned off");
		}
		return 0;
	}  
	temp_onoff_bak=temp_onoff;
	uint8_t *temp_buf=data_out;//gpio config idx(uint32_t),mode(uint8_t),value(int)
	int temp_buf_idx=1;//temp_buf[0]==0: normal data upload, temp_buf[0]==1: interrupt upload
	uint8_t previous_pin_mode,current_pin_mode;
	int pin_value=0;
	uint8_t is_logic_value;
	for(uint8_t i=0;i<sizeof(gpio_pins) / sizeof(gpio_pins[0]);i++)
	{
		pin_value=0;
		is_logic_value=1;
		if((is_esp!=0xff)&&((gpio_pins[i].is_esp!=is_esp)||(gpio_pins[i].pin!=gpio_no)))
			continue;
		previous_pin_mode=gpio_pins[i].previous_pin_mode;
		current_pin_mode=device_config_get_number(gpio_pins[i].pin_config);
		//clear adc if changed
		if((gpio_pins[i].adc_info!=NULL)&&(previous_pin_mode==3)&&(current_pin_mode!=3))
		{
			gpio_util_adc_read(gpio_pins[i].adc_info,true);
			util_gpio_init(gpio_pins[i].pin,GPIO_MODE_OUTPUT,GPIO_PULLDOWN_DISABLE,GPIO_PULLUP_DISABLE,0);
		}
		if(gpio_pins[i].is_esp)
		{
			switch(current_pin_mode)
			{
				case 1:
					if(current_pin_mode!=previous_pin_mode){
						util_gpio_init(gpio_pins[i].pin,GPIO_MODE_OUTPUT,GPIO_PULLDOWN_DISABLE,GPIO_PULLUP_DISABLE,0);
						out_pin_value_esp&= ~(1<<gpio_pins[i].pin);
					}
					pin_value=(out_pin_value_esp >> gpio_pins[i].pin) & 0x01;
					break;
				case 2:
					if(current_pin_mode!=previous_pin_mode){
						util_gpio_init(gpio_pins[i].pin,GPIO_MODE_OUTPUT,GPIO_PULLDOWN_DISABLE,GPIO_PULLUP_DISABLE,1);
						out_pin_value_esp|= (1<<gpio_pins[i].pin);
					}
					pin_value=(out_pin_value_esp >> gpio_pins[i].pin) & 0x01;
					break;
				case 30:
				case 31:
				case 32:
				case 33:
					if(gpio_pins[i].adc_info!=NULL)
					{
						gpio_pins[i].adc_info->atten=(current_pin_mode%10);
						if(gpio_util_adc_read(gpio_pins[i].adc_info,false)==0)
						{
							pin_value=gpio_pins[i].adc_info->value_voltage;
							is_logic_value=0;
						}
					}
					break;
				case 40:
				case 41:
				case 42:	
					if(current_pin_mode!=previous_pin_mode)
						gpio_util_isr_set(gpio_pins[i].pin_config,gpio_pins[i].pin,current_pin_mode,GPIO_INTR_POSEDGE);
					pin_value=gpio_get_level(gpio_pins[i].pin);
					break;
				case 50:	
				case 51:
				case 52:
					if(current_pin_mode!=previous_pin_mode)
						gpio_util_isr_set(gpio_pins[i].pin_config,gpio_pins[i].pin,current_pin_mode,GPIO_INTR_NEGEDGE);
					pin_value=gpio_get_level(gpio_pins[i].pin);
					break;
				case 60:	
				case 61:	
				case 62:	
					if(current_pin_mode!=previous_pin_mode)
						gpio_util_isr_set(gpio_pins[i].pin_config,gpio_pins[i].pin,current_pin_mode,GPIO_INTR_ANYEDGE);
					pin_value=gpio_get_level(gpio_pins[i].pin);
					break;
				case 70:	
				case 71:	
				case 72:						
					if(current_pin_mode!=previous_pin_mode)
						gpio_util_isr_set(gpio_pins[i].pin_config,gpio_pins[i].pin,current_pin_mode,0);
					pin_value=gpio_get_level(gpio_pins[i].pin);
					break;
				default:
					break;
			}
		}else {
			switch(current_pin_mode)
			{
				case 1:
					if(current_pin_mode!=previous_pin_mode){
						ch423_output(gpio_pins[i].pin, 0);
						out_pin_value_ext&= ~(1<<gpio_pins[i].pin);
					}
					
					break;
				case 2:
					if(current_pin_mode!=previous_pin_mode){
						ch423_output(gpio_pins[i].pin, 1);
						out_pin_value_ext|= (1<<gpio_pins[i].pin);
					}
					break;
				default:
					if(current_pin_mode!=previous_pin_mode)
						ch423_output(gpio_pins[i].pin, 0);
					break;
			}
			pin_value=(out_pin_value_ext >> gpio_pins[i].pin) & 0x01;	
		}
		if(current_pin_mode>0)
		{
			memcpy(temp_buf+temp_buf_idx,&gpio_pins[i].is_esp,sizeof(gpio_pins[i].is_esp));
			temp_buf_idx+=sizeof(gpio_pins[i].is_esp);
			memcpy(temp_buf+temp_buf_idx,&gpio_pins[i].pin,sizeof(gpio_pins[i].pin));
			temp_buf_idx+=sizeof(gpio_pins[i].pin);
			memcpy(temp_buf+temp_buf_idx,&current_pin_mode,sizeof(current_pin_mode));
			temp_buf_idx+=sizeof(current_pin_mode);
			memcpy(temp_buf+temp_buf_idx,&pin_value,sizeof(pin_value));
			temp_buf_idx+=sizeof(pin_value);		
			memcpy(temp_buf+temp_buf_idx,&is_logic_value,sizeof(is_logic_value));
			temp_buf_idx+=sizeof(is_logic_value);		
		}
		gpio_pins[i].previous_pin_mode=current_pin_mode;
	}
	if(temp_buf_idx>1)
	{
		data_out[0]=0;//has data
		*data_len_out=temp_buf_idx;
	}
	return 1;
}
int gpio_util_upload(void *parameter)
{
	uint8_t *data_out=user_malloc(512);
	int data_len_out=0,ret=-1;
	ret=gpio_util_process(data_out,&data_len_out,0xff,0);
	if(data_len_out>3)
	{
		data_out[0]=4;
		ret=tcp_send_command(
	      "gpio_isr",
	      data_container_create(1,COMMAND_REQ_GPIO,data_out, data_len_out,NULL),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	}else{
		user_free(data_out);
	}
	//ESP_LOGI(TAG, "gpio_util_process ret %d len %d",ret,data_len_out);
	return ret;
}
void gpio_util_property(uint8_t *data_out,int *data_len_out)
{
	gpio_util_process(data_out,data_len_out,0xff,0);
	data_out[0]=5;
}

void gpio_util_queue(uint32_t io_num)
{
	uint8_t pin=0,pin_level=0,pin_mode=0;
	uint16_t pin_config=0;
	pin_level=((io_num>>31)&0xff);
	pin_config=((io_num<<1)>>17)&0xffff;
	pin_mode=(io_num>>8)&0xff;
	pin=io_num&0xff;
	util_delay_ms(50);
	if(gpio_get_level(pin)==pin_level)
	{
		ESP_LOGI(TAG, "gpio_util_queue gpio isr pin %u level %u mode %u config %u",pin,pin_level,pin_mode,pin_config);
		//handle interrupt
		gpio_util_interrupt_upload(io_num);
    }
}
int command_operate_gpio_ext_set(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return)
{
	uint32_t idex=0;
	uint8_t gpio_ext_no=0,status=0;
	uint8_t *temp_data=command_data;	
	COPY_TO_VARIABLE(gpio_ext_no,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(status,temp_data,idex,command_data_len)
	data_return[0]=3;
	if(gpio_ext_no>CH423_OC_IO15)
	{
		sprintf((char *)data_return+1,"wrong ext gpio no %u",gpio_ext_no);
		return -1;
	}
	if(status>1)
	{
		sprintf((char *)data_return+1,"wrong status %u",status);
		return -2;
	}
	ESP_LOGI(TAG, "command_operate_gpi_ext_set gpio_ext_no %u to status %u",gpio_ext_no,status);
	esp_err_t ret=ch423_output(gpio_ext_no, status);
	if(status==0)
		out_pin_value_ext&= ~(1<<gpio_ext_no);

	else if(status==1)
		out_pin_value_ext|= (1<<gpio_ext_no);		
	if(ret==ESP_OK)
	{
		sprintf((char *)data_return+1,"ext gpio no %u set to %u",gpio_ext_no,status);
		return 0;
	}else {
		sprintf((char *)data_return+1,"ext gpio no %u set error %d",gpio_ext_no,ret);
		return -3;
	}
}
int command_operate_gpio_esp_set(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return)
{
	uint32_t idex=0;
	uint8_t gpio_esp_no=0,status=0,pull_up=0,pull_down=0;
	uint8_t *temp_data=command_data;	
	COPY_TO_VARIABLE(gpio_esp_no,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(status,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(pull_up,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(pull_down,temp_data,idex,command_data_len)
	data_return[0]=3;
	//0-21 35-48
	if(((gpio_esp_no>21)&&(gpio_esp_no<35))||(gpio_esp_no>48))
	{
		sprintf((char *)data_return+1,"wrong esp gpio no %u",gpio_esp_no);
		return -1;
	}
	if(status>1)
	{
		sprintf((char *)data_return+1,"wrong status %u",status);
		return -2;
	}
	if((pull_up>1)||(pull_down>1))
	{
		sprintf((char *)data_return+1,"wrong pull up/down %u/%u",pull_up,pull_down);
		return -3;
	}
	ESP_LOGI(TAG, "command_operate_gpio_esp_set gpio_esp_no %u to status %u pull up %u, pull down %u",gpio_esp_no,status,pull_up,pull_down);
	
	util_gpio_init(gpio_esp_no,GPIO_MODE_OUTPUT,pull_down,pull_up,status);
	if(status==0)
		out_pin_value_esp&= ~(1<<gpio_esp_no);
	else if(status==1)
		out_pin_value_esp|= (1<<gpio_esp_no);	
	sprintf((char *)data_return+1,"esp gpio no %u set to %u pull up %u, pull down %u",gpio_esp_no,status,pull_up,pull_down);
	return 0;
}
int command_operate_gpio_read(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return,int *data_return_len)
{
	uint32_t idex=0;
	uint8_t gpio_no=0,is_esp=0;
	uint8_t *temp_data=command_data;	
	COPY_TO_VARIABLE(is_esp,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(gpio_no,temp_data,idex,command_data_len)
	//ESP_LOGI(TAG, "command_operate_gpio_read gpio_no %u is_esp %u",gpio_no,is_esp);
	gpio_util_process(data_return,data_return_len,is_esp,gpio_no);
	data_return[0]=6;
	return 0;
}
