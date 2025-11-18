#include <stdio.h>
#include "esp_log.h"
#include "util.h"
#include "system_util.h"
#include "device_config.h"
#include "cloud_log.h"

#define CONFIG_ALL_ON

void define_user_config(void)
{
	if(define_config_max(CONFIG_NAME_MAX)<0) return;

#ifdef CONFIG_ALL_ON
    define_number_variable(MODULE_META,CONFIG_META_ONOFF,"meta_onoff",1,sizeof(uint8_t),0,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_META,CONFIG_META_UPLOAD_INTERVAL_SECONDS,"meta_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);    
   	define_number_variable(MODULE_ADC_BAT,CONFIG_ADC_BAT_ONOFF,"adc_bat_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
   	define_number_variable(MODULE_ADC_BAT,CONFIG_ADC_BAT_PARTIAL_PRESSURE,"adc_bat_partial_pressure",5000,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,1,0xffffffff); 
    define_number_variable(MODULE_ADC_BAT,CONFIG_ADC_BAT_UPLOAD_INTERVAL_SECONDS,"adc_bat_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);  
    define_number_variable(MODULE_LORA,CONFIG_LORA_MODE,"lora_mode",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);//0 off,1 rx
    define_number_variable(MODULE_LORA,CONFIG_LORA_FRE,"lora_fre",433000000,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,300000000,990000000);
    define_number_variable(MODULE_LORA,CONFIG_LORA_TX_OUTPUT_POWER,"lora_tx_power",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,3);
    define_number_variable(MODULE_LORA,CONFIG_LORA_BANDWIDTH,"lora_bandwidth",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,4,0,1,2,3);
    define_number_variable(MODULE_LORA,CONFIG_LORA_SPREADING_FACTOR,"lora_spreading_factor",10,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,7,12);
    define_number_variable(MODULE_LORA,CONFIG_LORA_CODINGRATE,"lora_codingrate",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,4,1,2,3,4);
    define_number_variable(MODULE_LORA,CONFIG_LORA_PREAMBLE_LENGTH,"lora_preamble_len",8,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,1,100);
    define_number_variable(MODULE_LORA,CONFIG_LORA_FIX_LENGTH_PAYLOAD_ON,"lora_fix_len_payload_on",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_LORA,CONFIG_LORA_IQ_INVERSION_ON,"lora_iq_inver_on",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_LORA,CONFIG_LORA_RX_TIMEOUT_VALUE,"lora_rx_timeout",5000,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,100,300000);
    define_number_variable(MODULE_LORA,CONFIG_LORA_RX_BUF_LEN,"lora_rx_buf_len",1024,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,16,4096);
    define_number_variable(MODULE_LORA,CONFIG_LORA_TX_TIMEOUT_VALUE,"lora_tx_timeout",5000,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,100,300000);
    define_number_variable(MODULE_LORA,CONFIG_LORA_UPLOAD_INTERVAL_SECONDS,"lora_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff); 
    define_number_variable(MODULE_UART,CONFIG_UART_ONOFF,"uart_onoff",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_UART,CONFIG_UART_UPLOAD_INTERVAL_SECONDS,"uart_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_UART,CONFIG_UART_BAUD_RATE,"uart_baud_rate",115200,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,1000,800000);
    define_number_variable(MODULE_UART,CONFIG_UART_TX_PIN,"uart_tx_pin",21,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,48);
    define_number_variable(MODULE_UART,CONFIG_UART_RX_PIN,"uart_rx_pin",2,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,48);
    define_number_variable(MODULE_GPIO,CONFIG_GPIO_ONOFF,"gpio_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_GPIO,CONFIG_GPIO_UPLOAD_INTERVAL_SECONDS,"gpio_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_4 ,"config_gpio_esp_4",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_5 ,"config_gpio_esp_5",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_6 ,"config_gpio_esp_6",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_7 ,"config_gpio_esp_7",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_15,"config_gpio_esp_15",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_16,"config_gpio_esp_16",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_17,"config_gpio_esp_17",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_18,"config_gpio_esp_18",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_8 ,"config_gpio_esp_8",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_3 ,"config_gpio_esp_3",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_46,"config_gpio_esp_46",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_9 ,"config_gpio_esp_9",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_10,"config_gpio_esp_10",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_11,"config_gpio_esp_11",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_14,"config_gpio_esp_14",33,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_21,"config_gpio_esp_21",60,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_48,"config_gpio_esp_48",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_45,"config_gpio_esp_45",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_0 ,"config_gpio_esp_0",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_38,"config_gpio_esp_38",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_39,"config_gpio_esp_39",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_40,"config_gpio_esp_40",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_41,"config_gpio_esp_41",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_42,"config_gpio_esp_42",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_2 ,"config_gpio_esp_2",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_ESP_1 ,"config_gpio_esp_1 ",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,18,1,2,30,31,32,33,40,41,42,50,51,52,60,61,62,70,71,72);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_IO7 ,"config_gpio_ext_io7",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC1 ,"config_gpio_ext_oc1",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2);  
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC5 ,"config_gpio_ext_oc5",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2);  
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC6 ,"config_gpio_ext_oc6",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2);  
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC9 ,"config_gpio_ext_oc9",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2);  
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC10,"config_gpio_ext_oc10",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC11,"config_gpio_ext_oc11",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC12,"config_gpio_ext_oc12",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC13,"config_gpio_ext_oc13",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_OC14,"config_gpio_ext_oc14",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2);
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_IO0 ,"config_gpio_ext_io0",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2);  
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_IO1 ,"config_gpio_ext_io1",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_IO2 ,"config_gpio_ext_io2",2,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2); 
	define_number_variable(MODULE_GPIO,CONFIG_GPIO_EXT_IO3 ,"config_gpio_ext_io3",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,1,2); 
    define_number_variable(MODULE_4G,CONFIG_4G_ONOFF,"4g_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_string_variable(MODULE_4G,CONFIG_4G_APN,"4g_apn","ctlte",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);
    define_number_variable(MODULE_4G,CONFIG_4G_AUTH_NEED,"4g_auth_need",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_4G,CONFIG_4G_AUTH_TYPE,"4g_auth_type",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,3,0,1,2);
    define_string_variable(MODULE_4G,CONFIG_4G_AUTH_USER,"4g_auth_user","user1",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);
    define_string_variable(MODULE_4G,CONFIG_4G_AUTH_PASSWORD,"4g_auth_password","password1",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);
    define_number_variable(MODULE_4G,CONFIG_4G_CPIN_SECONDS,"4g_cpin_seconds",60,sizeof(uint16_t),0,CONFIG_NUMBER_NORMAL,2,10,300);
    define_number_variable(MODULE_4G,CONFIG_4G_CSQ_SECONDS,"4g_csq_seconds",30,sizeof(uint16_t),0,CONFIG_NUMBER_NORMAL,2,10,300);
    define_number_variable(MODULE_4G,CONFIG_4G_CREG_SECONDS,"4g_creg_seconds",60,sizeof(uint16_t),0,CONFIG_NUMBER_NORMAL,2,10,300);
    define_number_variable(MODULE_WIFI,CONFIG_WIFI_MODE,"wifi_mode",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,4,0,1,2,3);
    define_string_variable(MODULE_WIFI,CONFIG_WIFI_AP_STA_SSID,"wifi_ap_sta_ssid","second",30,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,1,29);
    define_string_variable(MODULE_WIFI,CONFIG_WIFI_AP_STA_PASSWORD,"wifi_ap_sta_password","fuxiaohu1%",20,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,19);
    define_number_variable(MODULE_WIFI,CONFIG_WIFI_AP_CHANNEL,"wifi_ap_channel",6,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,1,14);
    define_number_variable(MODULE_WIFI,CONFIG_WIFI_AP_MAX_CONNECTION,"wifi_ap_max_connection",5,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,1,10);
    define_number_variable(MODULE_WIFI,CONFIG_WIFI_AP_SSID_HIDDEN,"wifi_ap_ssid_hidden",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_WIFI,CONFIG_WIFI_AP_DNS_UPDATE_SECONDS,"wifi_ap_dns_update_seconds",20,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,1,0xffffffff);
    define_string_variable(MODULE_WIFI,CONFIG_WIFI_AP_DNS_DEFAULT,"wifi_ap_dns_default","0",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_IP_URL,0,49);
    define_number_variable(MODULE_TM7705,CONFIG_TM7705_ONOFF,"tm7705_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_TM7705,CONFIG_TM7705_UPLOAD_INTERVAL_SECONDS,"tm7705_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_TM7705,CONFIG_TM7705_PINS_ONOFF,"tm7705_pins_onoff",3,sizeof(uint8_t),0,CONFIG_NUMBER_NORMAL,2,0,3);
    define_number_variable(MODULE_TM7705,CONFIG_TM7705_INIT_TIMES,"tm7705_init_times",2,sizeof(uint16_t),0,CONFIG_NUMBER_NORMAL,2,1,300);
    define_number_variable(MODULE_TM7705,CONFIG_TM7705_REG_FREQ,"tm7705_reg_freq",0x04,sizeof(uint8_t),0,CONFIG_NUMBER_NORMAL,2,0,255);
    define_number_variable(MODULE_TM7705,CONFIG_TM7705_REG_CONFIG,"tm7705_reg_config",0x64,sizeof(uint8_t),0,CONFIG_NUMBER_NORMAL,2,0,255);
    define_number_variable(MODULE_CAMERA,CONFIG_CAMERA_ONOFF,"camera_onoff",1,sizeof(uint8_t),0,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_CAMERA,CONFIG_CAMERA_UPLOAD_INTERVAL_SECONDS,"camera_upload_interval_seconds",200,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_CAMERA,CONFIG_CAMERA_SIZE,"camera_size",21,sizeof(uint8_t),0,CONFIG_NUMBER_NORMAL,2,0,24);
    define_number_variable(MODULE_CAMERA,CONFIG_CAMERA_QUALITY,"camera_quality",45,sizeof(uint8_t),0,CONFIG_NUMBER_NORMAL,2,0,63);
    define_number_variable(MODULE_CAMERA,CONFIG_CAMERA_AEC_GAIN,"camera_aec_gain",2000,sizeof(uint16_t),0,CONFIG_NUMBER_NORMAL,2,0,3000);
    define_number_variable(MODULE_W5500,CONFIG_W5500_MODE,"w5500_mode",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,4,0,1,2,3);
    define_string_variable(MODULE_W5500,CONFIG_W5500_IP,"w5500_ip","192.168.18.177",16,CONFIG_FLAG_REBOOT,CONFIG_STRING_IP_V4,7,15);
    define_string_variable(MODULE_W5500,CONFIG_W5500_SUBNET_MASK,"w5500_subnet_mask","255.255.255.0",16,CONFIG_FLAG_REBOOT,CONFIG_STRING_IP_V4,7,15);
    define_string_variable(MODULE_W5500,CONFIG_W5500_GATEWAY,"w5500_gateway","192.168.18.1",16,CONFIG_FLAG_REBOOT,CONFIG_STRING_IP_V4,7,15);
    define_string_variable(MODULE_W5500,CONFIG_W5500_DNS1,"w5500_dns1","114.114.114.114",16,CONFIG_FLAG_REBOOT,CONFIG_STRING_IP_V4,7,15);
    define_string_variable(MODULE_W5500,CONFIG_W5500_DNS2,"w5500_dns2","8.8.8.8",16,CONFIG_FLAG_REBOOT,CONFIG_STRING_IP_V4,7,15);
    define_number_variable(MODULE_CONFIG,CONFIG_CONFIG_UPLOAD_ONOFF,"config_upload_onoff",1,sizeof(uint8_t),0,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_CONFIG,CONFIG_CONFIG_UPLOAD_INTERVAL_SECONDS,"config_upload_interval_seconds",200,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
	define_number_variable(MODULE_FORWARD,CONFIG_FORWARD_UPLOAD_ONOFF,"forward_onoff",1,sizeof(uint8_t),0,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_FORWARD,CONFIG_FORWARD_UPLOAD_INTERVAL_SECONDS,"forward_upload_interval_seconds",200,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_AHT20,CONFIG_AHT20_ONOFF,"aht20_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_AHT20,CONFIG_AHT20_UPLOAD_INTERVAL_SECONDS,"aht20_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_SPL06,CONFIG_SPL06_ONOFF,"spl06_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_SPL06,CONFIG_SPL06_UPLOAD_INTERVAL_SECONDS,"spl06_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_BLE,CONFIG_BLE_PASSWORD,"ble_password",237689,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,999999);
    define_number_variable(MODULE_RS485,CONFIG_RS485_ONOFF,"rs485_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_RS485,CONFIG_RS485_UPLOAD_INTERVAL_SECONDS,"rs485_upload_interval_seconds",100,sizeof(uint32_t),0,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_RS485,CONFIG_RS485_BAUD_RATE,"rs485_baud_rate",115200,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,1000,800000);
    define_number_variable(MODULE_RS485,CONFIG_RS485_TX_PIN,"rs485_tx_pin",21,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,48);
    define_number_variable(MODULE_RS485,CONFIG_RS485_RX_PIN,"rs485_rx_pin",2,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,48);
    define_number_variable(MODULE_PIR,CONFIG_PIR_MODE,"pir_mode",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,4,0,1,2,3);
    define_number_variable(MODULE_PIR,CONFIG_PIR_UPLOAD_INTERVAL_SECONDS,"pir_upload_interval_seconds",200,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_PIR,CONFIG_PIR_VALID_TIMES,"pir_valid_times",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,255);   
    define_number_variable(MODULE_PIR,CONFIG_PIR_NOT_REPEAT_UPLOAD_SECONDS,"pir_not_repeat_upload_seconds",0,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_PIR,CONFIG_PIR_GPIO_NUM,"pir_gpio_num",14,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,48);
    define_number_variable(MODULE_PIR,CONFIG_PIR_ACTION,"pir_action",0,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,255);
    define_number_variable(MODULE_PIR,CONFIG_PIR_ACTION_TIMES,"pir_action_times",3,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_PIR,CONFIG_PIR_ACTION_INTERVAL_SECONDS,"pir_action_interval_seconds",2,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_THERMAL,CONFIG_THERMAL_ONOFF,"thermal_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_THERMAL,CONFIG_THERMAL_UPLOAD_INTERVAL_SECONDS,"thermal_upload_interval_seconds",100,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_THERMAL,CONFIG_THERMAL_EMISSIVITY,"thermal_emissivity",95,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_THERMAL,CONFIG_THERMAL_SHIFT,"thermal_shift",800,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_ONOFF,"ultrasonic_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_UPLOAD_INTERVAL_SECONDS,"ultrasonic_upload_interval_seconds",100,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_PIN_TRIG,"ultrasonic_pin_trig",16,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffff);
 	define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_PIN_ECHO,"ultrasonic_pin_echo",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffff);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_MEASURE_MILLI_SECONDS,"ultrasonic_measure_milli_seconds",1,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,1,0xffffffff);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_COMPENSATION_MODE,"ultrasonic_compensation_mode",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,3,0,1,2);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_COMPENSATION_VALUE,"ultrasonic_compensation_value",70,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffff);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_TRIGGER_TIMES,"ultrasonic_trigger_times",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffff);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_TRIGGER_MIN,"ultrasonic_trigger_min",0,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);
    define_number_variable(MODULE_ULTRASONIC,CONFIG_ULTRASONIC_TRIGGER_MAX,"ultrasonic_trigger_max",0,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,0,0xffffffff);    
 	define_number_variable(MODULE_DTU,CONFIG_DTU_ONOFF,"dtu_onoff",1,sizeof(uint8_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_ENUM,2,0,1);
    define_string_variable(MODULE_DTU,CONFIG_DTU_MQTT_URL,"dtu_mqtt_url","mqtts://139.196.107.200",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);
 	define_number_variable(MODULE_DTU,CONFIG_DTU_MQTT_PORT,"dtu_mqtt_port",1883,sizeof(uint32_t),CONFIG_FLAG_REBOOT,CONFIG_NUMBER_NORMAL,2,10,0xffffffff);
    define_string_variable(MODULE_DTU,CONFIG_DTU_MQTT_USER,"dtu_mqtt_user","user",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);
    define_string_variable(MODULE_DTU,CONFIG_DTU_MQTT_PASSWORD,"dtu_mqtt_password","quitto12vers",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);
	define_string_variable(MODULE_DTU,CONFIG_DTU_MQTT_HINT_NAME,"dtu_mqtt_hint_name","hint",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);
	define_string_variable(MODULE_DTU,CONFIG_DTU_MQTT_HINT_HEX_KEY,"dtu_mqtt_hint_hex_key","BAD123",50,CONFIG_FLAG_REBOOT,CONFIG_STRING_NORMAL,0,49);

#endif
}
