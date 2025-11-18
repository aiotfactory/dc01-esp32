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
#include <ultrasonic_util.h>
#include "esp_timer.h"
#include "math.h"
#include "device_config.h"
		
static uint32_t config_trigger_min=0,config_trigger_max=0,config_measure_milli_seconds=0;
static uint8_t config_compensation_mode=0,config_trigger_times=0,config_pin_trig=0,config_pin_echo=0,config_compensation_value=0;
extern int temperature_aht20,humidity_aht20;
extern int temperature_spl06,pressure_spl06;


static const char *TAG = "ultrasonic_util";

#define ULTRASONIC_CACHE_NUM 20
static uint16_t cache_max=0,cache_min=0;
static uint32_t cache_idx=0,total_times=0;
static uint16_t cache[ULTRASONIC_CACHE_NUM]={0};
static QueueHandle_t ultrasonic_queue_v=NULL;
static SemaphoreHandle_t ultrasonic_semaphore_handle=NULL;
static uint32_t doing=0;

static void IRAM_ATTR ultrasonic_util_isr_handler(void* arg)
{
	static uint8_t times=0;
	static int64_t time1=-1;
	int64_t time_tmp=esp_timer_get_time();
    uint8_t gpio_level=gpio_get_level(((uint32_t) arg)&0xff);
	 if(gpio_level){
		time1 = time_tmp;
		times=0;	 	
	 }else{
		if(times==1)
		{
			uint32_t temp_value=time_tmp-time1;
			BaseType_t xHigherPriorityTaskWoken;
		    xQueueSendFromISR(ultrasonic_queue_v, &temp_value, &xHigherPriorityTaskWoken);
		    if( xHigherPriorityTaskWoken )
		 	{
		       portYIELD_FROM_ISR ();
		  	}
		  	doing=0;
 		}
 	}
 	times++;
}

static float speed_sound1(float temperature ) {
    float speed_of_sound = (331.45+0.61*temperature);
    return speed_of_sound;
}
static float speed_sound2(float temperature, float relative_humidity) {
    float rh = relative_humidity / 100.0;
    float speed_of_sound = 331.3 * sqrt(1 + temperature / 273.15) *
                         (1 + rh * (0.0124 - 0.00018 * temperature));
    
    return speed_of_sound;
}

void ultrasonic_queue(void)
{
	uint32_t temp_value=0;
	float distance=0;
	int btemperature_aht20,bhumidity_aht20;
	int btemperature_spl06;
	static uint8_t temp_trigger_times=0;
	
	if(ultrasonic_queue_v==NULL)
		return;
	if(xQueueReceive(ultrasonic_queue_v, &temp_value,0)) 
	{
		distance=343.65;//20摄氏度建议值
		if(config_compensation_mode>0)
		{
			if(config_compensation_mode==1)
			{
				variable_set(&btemperature_aht20,&temperature_aht20, sizeof(int));
				variable_set(&bhumidity_aht20,&humidity_aht20, sizeof(int));
				variable_set(&btemperature_spl06,&temperature_spl06, sizeof(int));				
				btemperature_aht20=(btemperature_aht20==-99999999?btemperature_spl06:btemperature_aht20);
				if(btemperature_aht20>-99999999)
				{
					btemperature_aht20=(btemperature_aht20/100-2);
					if(bhumidity_aht20>-99999999){
						bhumidity_aht20=bhumidity_aht20/100;
						distance=speed_sound2(btemperature_aht20,bhumidity_aht20);
					}else {
						distance=speed_sound1(btemperature_aht20);
					}
				}
			}else if(config_compensation_mode==2)
			{
				distance=speed_sound1((float)config_compensation_value-50);
			}
		}
		distance=((distance*temp_value)/2)/1000;
		if(distance<30000 && distance > 1)
		{	
			if(xSemaphoreTake(ultrasonic_semaphore_handle,portMAX_DELAY)==pdTRUE)
			{
				cache[cache_idx%ULTRASONIC_CACHE_NUM]=distance;
	 			cache_idx++;
	 			if(cache_max==0 || cache_max<distance) cache_max=distance;
	 			if(cache_min==0 || distance<cache_min) cache_min=distance;
	
	 			total_times++;
	 			xSemaphoreGive(ultrasonic_semaphore_handle);
	 			
	 		}
	 		
	 		if(config_trigger_max>0 && config_trigger_max>config_trigger_min && distance>=config_trigger_min && distance<=config_trigger_max)
	 		{
				temp_trigger_times++;
				if(temp_trigger_times>=config_trigger_times){
					temp_trigger_times=0;
					ultrasonic_upload((void *)2);
				}
			}else 
				temp_trigger_times=0;
		}
	}
}
static void pin_trig(uint8_t onoff)
{
	static uint8_t init=0;
	uint8_t pin_value=config_pin_trig & 0x7f;
	uint8_t pin_type = config_pin_trig>>0x07;
	if(pin_type) //esp
	{
		if(init==0){				
			init=1;
			util_gpio_init(pin_value,GPIO_MODE_OUTPUT,1,0,0);
		}else {
			gpio_set_level(pin_value,onoff);
		}
	}else {
		ch423_output(pin_value, onoff);
	}
}
static void ultrasonic_util_isr_set(void)
{
	uint8_t config_mode=GPIO_INTR_ANYEDGE;
	gpio_config_t io_conf = {};
	io_conf.intr_type = config_mode;
	io_conf.pin_bit_mask = (1ULL << config_pin_echo);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en=1;
    gpio_config(&io_conf);
    gpio_isr_handler_add(config_pin_echo, ultrasonic_util_isr_handler, (void *)((1 << 30) | (config_mode << 8) | config_pin_echo));
    ESP_LOGI(TAG, "ultrasonic_util_isr_set pin %u int_type %u",config_pin_echo,config_mode);
}

static void periodic_timer_callback(void* arg)
{
	static int64_t time_tmp=-1;
    if(doing==0)
    {
		doing=1;
		time_tmp=esp_timer_get_time();
    	pin_trig(1);
   		esp_rom_delay_us(12);
    	pin_trig(0);
    }else if(doing && esp_timer_get_time()>(time_tmp+3*1000*1000))
    	doing=0;
}
static uint8_t *ultrasonic_exe(uint8_t type,int *data_len)
{
    static uint8_t isr_init=0;
	int temp_buf_idx=0;
	uint8_t *temp_buf;
	*data_len=0;
	if(isr_init==0)
	{
		isr_init=1;
		ultrasonic_queue_v = xQueueCreate(10, sizeof(uint32_t));
		power_onoff_spi(1);
		util_delay_ms(10);
		pin_trig(0);
		config_compensation_mode=device_config_get_number(CONFIG_ULTRASONIC_COMPENSATION_MODE);
		config_compensation_value=device_config_get_number(CONFIG_ULTRASONIC_COMPENSATION_VALUE);
		config_trigger_times=device_config_get_number(CONFIG_ULTRASONIC_TRIGGER_TIMES);
		config_trigger_min=device_config_get_number(CONFIG_ULTRASONIC_TRIGGER_MIN);
		config_trigger_max=device_config_get_number(CONFIG_ULTRASONIC_TRIGGER_MAX);
		config_measure_milli_seconds=device_config_get_number(CONFIG_ULTRASONIC_MEASURE_MILLI_SECONDS);		
		config_pin_trig=device_config_get_number(CONFIG_ULTRASONIC_PIN_TRIG);
		config_pin_echo=device_config_get_number(CONFIG_ULTRASONIC_PIN_ECHO);				
		ultrasonic_semaphore_handle=xSemaphoreCreateMutex();
		util_delay_ms(10);
		ultrasonic_util_isr_set();
		util_delay_ms(10);
		const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            .name = "ultrasonic_periodic"
    	};
    	esp_timer_handle_t periodic_timer;
    	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, config_measure_milli_seconds*1000));
	}
	
	if(type!=2)//no need for trigger
	{
		ultrasonic_queue();
		for(int i=0;i<30;i++)
		{
			util_delay_ms(20);
			ultrasonic_queue();
			if(cache_idx>0) break;
		}
	}
	
	if(cache_idx==0)
		return NULL;
	temp_buf=user_malloc(sizeof(cache)+16);
	
	COPY_TO_BUF_NO_CHECK(type,temp_buf,temp_buf_idx);
	if(xSemaphoreTake(ultrasonic_semaphore_handle,portMAX_DELAY)==pdTRUE)
	{
		COPY_TO_BUF_NO_CHECK(total_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_NO_CHECK(cache_max,temp_buf,temp_buf_idx);
		COPY_TO_BUF_NO_CHECK(cache_min,temp_buf,temp_buf_idx);
		COPY_TO_BUF_NO_CHECK(cache_idx,temp_buf,temp_buf_idx);
		COPY_TO_BUF_NO_CHECK(cache,temp_buf,temp_buf_idx);
		bzero(cache,sizeof(cache));
		cache_idx=0;
		xSemaphoreGive(ultrasonic_semaphore_handle);
	}
	*data_len=temp_buf_idx;
	return temp_buf;

}
int ultrasonic_upload(void *parameter)
{
	int temp_buf_idx=0;
	int ret=-1;
	uint8_t type=(parameter==NULL?0:(uint32_t)parameter);
	uint8_t *temp_buf=ultrasonic_exe(type,&temp_buf_idx);
	if(temp_buf==NULL)
		return ret;
	ret=tcp_send_command(
      "ultrasonic",
      data_container_create(1,COMMAND_REQ_ULTRASONIC,temp_buf, temp_buf_idx,NULL),
      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
      SOCKET_HANDLER_ONE_TIME, 0,NULL);

	
	return ret;
}
int ultrasonic_request(data_send_element *data_element)
{
	if(device_config_get_number(CONFIG_ULTRASONIC_ONOFF)==0)
		return -1;
		
	int temp_buf_idx=0;
	uint8_t *temp_buf=ultrasonic_exe(1,&temp_buf_idx);
	if(temp_buf==NULL)
		return -1;
	data_element->data=temp_buf;
	data_element->data_len=temp_buf_idx;
	data_element->need_free=1;
	return 0;
}
