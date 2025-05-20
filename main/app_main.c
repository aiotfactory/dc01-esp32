#include "util.h"
#include "ch423.h"
#include "camera_util.h"
#include "ethernet_util.h"
#include "tm7705_util.h"
#include "spi_util.h"
#include "lora_util.h"
#include "module_4g_util.h"
#include "network_util.h"
#include "device_config.h"
#include "meta_util.h"
#include "gpio_util.h"
#include "pir_util.h"
#include "thermal_util.h"
#include "uart_util.h"
#include "rs485_util.h"
#include "bat_adc.h"
#include "ultrasonic_util.h"
#include "system_util.h"
#include "config_upload_util.h"
#include "forward_util.h"
#include "cloud_log.h"
#include "aht20.h"
#include "spl06.h"
#include "ble_util.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"

static const char *TAG = "app_main";

#define LOOP_INTERVAL_TICKS 1000/portTICK_PERIOD_MS
 
typedef struct {
	uint8_t  status;
	uint32_t module;
	char     *task_name;
	uint32_t exe_times;
	uint32_t exe_times_failed;
    uint32_t exe_last_seconds;
    uint32_t task_upload_interval_idex;
    int      (*task_func)(void *parameter);
    void     *parameter;
    int result_last;
} task_info;

#define UTIL_TASK_ADD(idx,module1,callback_func,onoff,interval_seconds,parameter1)\
	if(device_config_get_number(onoff)>0){\
		all_tasks[idx].status=1;\
		all_tasks[idx].module=module1;\
		all_tasks[idx].task_name=#callback_func;\
		all_tasks[idx].task_func=callback_func;\
		all_tasks[idx].task_upload_interval_idex=interval_seconds;\
		all_tasks[idx].exe_last_seconds=0;\
		all_tasks[idx].exe_times=0;\
		all_tasks[idx].exe_times_failed=0;\
		all_tasks[idx].parameter=parameter1;\
		all_tasks[idx].result_last=0;\
		idx+=1;\
	}
	
void loop_wait_on_interrupt(uint32_t max_tickets)
{
	uint32_t ticks1=xTaskGetTickCount(),ticks2=0,io_num=0;
	do
	{		
		if(gpio_evt_queue!=NULL && xQueueReceive(gpio_evt_queue, &io_num,0)) 
		{
			if(io_num<<30)
				pir_util_queue(io_num);
			else
				gpio_util_queue(io_num);
		}
		
		uart_util_queue();
		rs485_util_queue();
		ultrasonic_queue();
		ticks2 = xTaskGetTickCount();
		ticks2=ticks2>=ticks1?ticks2-ticks1:0xffffffff-ticks1+ticks2;
	}while(max_tickets>ticks2);	
	
}
/**
 * 设备上电后首先调用的入口函数�? * 该函数是整个应用程序的起点，设备在上电或复位后会自动调用此函数以初始化系统并启动主要功能�? * 在该函数中，通常会完成系统的基础配置、硬件初始化、任务创建以及启动其他必要的服务或模块�? * 开发者可以在该函数中实现自定义的初始化逻辑，例如设置回调函数、启动定时器或注册事件处理程序�? * 
 * This function is the entry point of the entire application and is automatically called after the device is powered on or reset.
 * It is responsible for initializing the system and starting the main functionalities. Typically, basic system configuration, 
 * hardware initialization, task creation, and the startup of other necessary services or modules are performed within this function.
 * Developers can implement custom initialization logic here, such as setting callback functions, starting timers, or registering event handlers.
 */
 //chip send powerstart-4092-4092-4092-1-1-1-powerend
void app_main(void)
{
    system_config_t config = {
	    .callback = define_user_config,
	    .config_version = 1,
	    .server_host = "139.196.107.200",
	    .server_port = 7000,
	    .upload_timeout_seconds = 30,
	    .keep_alive_idle = 60,
	    .keep_alive_cnt = 3,
	    .keep_alive_interval = 10,
	    .no_communicate_seconds = 1800,
	    .module_max = MODULE_MAX,
	    .sleep_for_x_seconds=0,  
        .sleep_after_running_x_seconds=0,
	    .core_log = 0,
	    .mem_debug = 0,
	    .cloud_log = 0,
    };
    
  	system_init(&config);//don't comment it out or move to other position
	//config_print_variable("config");
	//print_chip_info();
	//print_mem_info();
	
	ble_start();//allow user to config device by BLE
	ch423_init();//init ch423 to turn on ext gpio
	ESP_LOGI(TAG, "gpio ext module is turned on");
	spi_cs_pins_default();//set all spi cs pins to high
	ESP_LOGI(TAG, "all spi cs pins are pulled up");
	ESP_LOGI(TAG, "wait for 3 seconds to ensure all the electricity from the capacitor is released");
	util_delay_ms(3000);
	
	app_evt_group_hdl = xEventGroupCreate();//create bits for sync
    assert(app_evt_group_hdl != NULL);
    
    if(device_config_get_number(CONFIG_GPIO_ONOFF)|device_config_get_number(CONFIG_PIR_MODE))
		gpio_evt_queue = xQueueCreate(20, sizeof(uint32_t));
	
	//turn on i2c power
	power_onoff_i2c(1); //since tm7705 is using i2c power, so you need to turn it on, or it will impact other spi devices	    
	
	//ultrasonic_upload(NULL);
	
	//4g
	if(device_config_get_number(CONFIG_4G_ONOFF))
	{
		util_reset_4g(1);//set reset pin of 4g module to high
		ESP_LOGI(TAG, "4g module is powered on");
		power_onoff_4g(1);//turn on power of 4g module
		module_4g_start(30);
	}else {
		ESP_LOGI(TAG, "4g module is off");
	}

	//ethernet	
	if(device_config_get_number(CONFIG_W5500_MODE))
	{
		 power_onoff_eth(1);
		if(ethernet_check())
			ethernet_start();  	
		else
 			ESP_LOGW(TAG, "ethernet module not work");
	}
	
	esp_netif_t ** wifi_netif=module_wifi_start();
	
	system_start();//don't comment it out or move to other position
	
	task_info *all_tasks=user_malloc(sizeof(task_info)*16);
	uint8_t task_idx=0;
	//uint32_t task_start_seconds=0;
	int task_ret=0;
	//put it as first task, so that interrupt can be processed immediately
	
	UTIL_TASK_ADD(task_idx,MODULE_GPIO,gpio_util_upload,CONFIG_GPIO_ONOFF,CONFIG_GPIO_UPLOAD_INTERVAL_SECONDS,NULL)	
	UTIL_TASK_ADD(task_idx,MODULE_UART,uart_util_upload,CONFIG_UART_ONOFF,CONFIG_UART_UPLOAD_INTERVAL_SECONDS,NULL)	
	UTIL_TASK_ADD(task_idx,MODULE_RS485,rs485_util_upload,CONFIG_RS485_ONOFF,CONFIG_RS485_UPLOAD_INTERVAL_SECONDS,NULL)	
	UTIL_TASK_ADD(task_idx,MODULE_META,meta_upload,CONFIG_META_ONOFF,CONFIG_META_UPLOAD_INTERVAL_SECONDS,NULL)	
	UTIL_TASK_ADD(task_idx,MODULE_TM7705,tm7705_upload,CONFIG_TM7705_ONOFF,CONFIG_TM7705_UPLOAD_INTERVAL_SECONDS,NULL)	
	UTIL_TASK_ADD(task_idx,MODULE_ADC_BAT,bat_adc_upload,CONFIG_ADC_BAT_ONOFF,CONFIG_ADC_BAT_UPLOAD_INTERVAL_SECONDS,NULL)	
	UTIL_TASK_ADD(task_idx,MODULE_LORA,lora_util_upload,CONFIG_LORA_MODE,CONFIG_LORA_UPLOAD_INTERVAL_SECONDS,NULL)	
    UTIL_TASK_ADD(task_idx,MODULE_CONFIG,config_upload,CONFIG_CONFIG_UPLOAD_ONOFF,CONFIG_CONFIG_UPLOAD_INTERVAL_SECONDS,NULL)	
    UTIL_TASK_ADD(task_idx,MODULE_FORWARD,forward_upload,CONFIG_FORWARD_UPLOAD_ONOFF,CONFIG_FORWARD_UPLOAD_INTERVAL_SECONDS,NULL)	
    UTIL_TASK_ADD(task_idx,MODULE_AHT20,aht20_upload,CONFIG_AHT20_ONOFF,CONFIG_AHT20_UPLOAD_INTERVAL_SECONDS,NULL)	
    UTIL_TASK_ADD(task_idx,MODULE_SPL06,spl06_upload,CONFIG_SPL06_ONOFF,CONFIG_SPL06_UPLOAD_INTERVAL_SECONDS,NULL)	
    UTIL_TASK_ADD(task_idx,MODULE_PIR,pir_upload,CONFIG_PIR_MODE,CONFIG_PIR_UPLOAD_INTERVAL_SECONDS,NULL)	
    UTIL_TASK_ADD(task_idx,MODULE_THERMAL,thermal_upload,CONFIG_THERMAL_ONOFF,CONFIG_THERMAL_UPLOAD_INTERVAL_SECONDS,NULL)
    UTIL_TASK_ADD(task_idx,MODULE_ULTRASONIC,ultrasonic_upload,CONFIG_ULTRASONIC_ONOFF,CONFIG_ULTRASONIC_UPLOAD_INTERVAL_SECONDS,NULL)
    UTIL_TASK_ADD(task_idx,MODULE_CAMERA,camera_upload,CONFIG_CAMERA_ONOFF,CONFIG_CAMERA_UPLOAD_INTERVAL_SECONDS,NULL)	
    
	if((wifi_netif!=NULL)&&(device_config_get_number(CONFIG_WIFI_MODE)==WIFI_MODE_AP)){
		UTIL_TASK_ADD(task_idx,MODULE_WIFI,module_wifi_set_dns,CONFIG_WIFI_MODE,CONFIG_WIFI_AP_DNS_UPDATE_SECONDS,wifi_netif[1])	
	}
	print_mem_info();

	uint32_t loop_times=0,task_success_times=0,task_fail_times=0,temp_interval=0;
	while(1)
	{
		loop_times++;
		#if 1
		for(uint8_t i=0;(i<task_idx)&&(all_tasks[i].status>0);i++)
		{
			if(task_ret==-102) //network busy
			{
				util_delay_ms(10*1000);
				if(i>0) i++;//retry
			}
				
			loop_wait_on_interrupt(0);
			temp_interval=device_config_get_number(all_tasks[i].task_upload_interval_idex);
			if((all_tasks[i].status)&&((all_tasks[i].exe_times==0)||((temp_interval>0)&&(util_get_run_seconds()-all_tasks[i].exe_last_seconds)>temp_interval)))
			{
				//uint32_t task_start_seconds=util_get_run_seconds();
				//ESP_LOGI(TAG, "task start %s",all_tasks[i].task_name);
				task_ret=-1;
				if(module_lock("module",all_tasks[i].module, portMAX_DELAY) == pdTRUE)
				{
					task_ret=all_tasks[i].task_func(all_tasks[i].parameter);
					module_unlock("module",all_tasks[i].module);
				}
				if(task_ret<0){
					all_tasks[i].exe_times_failed=all_tasks[i].exe_times_failed+1;
					task_fail_times++;
				}
				else{
 					task_success_times++;
 					led_flash(400);
 				}
				all_tasks[i].exe_times=all_tasks[i].exe_times+1;
				all_tasks[i].result_last=task_ret;
				all_tasks[i].exe_last_seconds=util_get_run_seconds();
				//ESP_LOGI(TAG, "task end %s seconds %lu times %lu ret %d",all_tasks[i].task_name,all_tasks[i].exe_last_seconds-task_start_seconds,all_tasks[i].exe_times,task_ret);

				if(config.mem_debug)
				{
					ESP_LOGI(TAG, "main stack high water mark %d", uxTaskGetStackHighWaterMark(NULL));
					print_mem_info();
					user_malloc_print(2);
				}
			}
		}
		#endif
		loop_wait_on_interrupt(LOOP_INTERVAL_TICKS);
		
		if(util_get_run_seconds()>10*60)//will stop after 10 minutes from rebooting
			ble_stop();
		ESP_LOGI(TAG, "loop times %lu task success %lu fail %lu seconds %lu",loop_times,task_success_times,task_fail_times,util_get_run_seconds());
	}
}

