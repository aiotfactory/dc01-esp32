/* MQTT over SSL Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "portmacro.h"
#include "util.h"
#include "system_util.h"
#include "device_config.h"
#include "mqtt_util.h"
#include "ble_central_util.h"

typedef struct mqtt_data_upload {
    char *topic;
    uint8_t *data;
    int data_len;
} mqtt_data_upload_t;


static const char *TAG = "mqtts_util";

static esp_mqtt_client_handle_t mqtt_client = NULL;        
static QueueHandle_t        g_queue_mqtt_upload  = NULL;
static bool connected = false;
static uint32_t connect_times = 0;
static uint32_t mqtt_heartbeat = 0;
static uint32_t heartbeat_times = 0;

static void mqtt_data_free(mqtt_data_upload_t *data) {
	if(data!=NULL)
	{
		if(data->data!=NULL)
			user_free(data->data);
		if(data->topic!=NULL)
			user_free(data->topic);
		user_free(data);			
	}
}
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
        	connected = true;
          
			char *topic_temp = user_malloc(strlen(TOPIC_BLE_DTU_COMMAND)+32);
			sprintf(topic_temp,"/dc01/%02x%02x%02x%02x%02x%02x/%s",rt_device_mac[0],rt_device_mac[1],rt_device_mac[2],rt_device_mac[3],rt_device_mac[4],rt_device_mac[5],TOPIC_BLE_DTU_COMMAND);
            msg_id = esp_mqtt_client_subscribe(client, topic_temp, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d %s", msg_id,topic_temp);
            user_free(topic_temp);
            if(msg_id<0) {
            	esp_mqtt_client_disconnect(client);
            }
           	if(connect_times==0) 
           	{
            	blecent_cloud_upload_status(1,"restart");
            } else {
				char *msg = user_malloc(128);
				sprintf(msg,"reconnect %lu times",connect_times);
				blecent_cloud_upload_status(2,msg);
				user_free(msg);
			}
			connect_times++;
			
            break;
        case MQTT_EVENT_DISCONNECTED:
        	connected = false;
            ESP_LOGE(TAG, "mqtt_event_handler disconn");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            //ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            //ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            //printf("recv topic %.*s data len %d\r\n", event->topic_len, event->topic,event->data_len);
            //print_format(NULL,0,"%02x","recv data ", "\r\n",(uint8_t *)event->data,event->data_len);
            //printf("recv data %.*s\r\n", event->data_len, event->data);
            blecent_cloud_command(event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "mqtt_event_handler error");
            break;
        default:
            ESP_LOGI(TAG, "mqtt_event_handler other event id:%d", event->event_id);
            break;
    }
}

int mqtt_send(char *topic,uint8_t *data,uint32_t data_len) 
{
	mqtt_data_upload_t *mqtt_data = user_malloc(sizeof(mqtt_data_upload_t));
	mqtt_data->topic = user_malloc(strlen(topic)+1);
	strcat(mqtt_data->topic,topic);
	mqtt_data->data = user_malloc(data_len);
	memcpy(mqtt_data->data,data,data_len);
	mqtt_data->data_len = data_len;
	if (xQueueSend(g_queue_mqtt_upload, &mqtt_data, 0) == pdFALSE) 
	{
		mqtt_data_free(mqtt_data);
    	ESP_LOGW(TAG, "mqtt_send failed %s len %lu",topic,data_len);
    	return -1;
	}

	return 0;
}

static void mqtt_task(void *pvParameters)
{
	mqtt_data_upload_t *mqtt_data;	
	BaseType_t recv_type;
    while (1) 
    {	
		if(mqtt_heartbeat>0)
			recv_type = xQueueReceive(g_queue_mqtt_upload, &mqtt_data, pdMS_TO_TICKS(mqtt_heartbeat*1000));
		else
			recv_type = xQueueReceive(g_queue_mqtt_upload, &mqtt_data, portMAX_DELAY);
			
		if(recv_type == pdTRUE)
		{
			if(connected) 
			{
				char *topic_temp = user_malloc(strlen(mqtt_data->topic)+32);
				sprintf(topic_temp,"/dc01/%02x%02x%02x%02x%02x%02x/%s",rt_device_mac[0],rt_device_mac[1],rt_device_mac[2],rt_device_mac[3],rt_device_mac[4],rt_device_mac[5],mqtt_data->topic);
				int msg_id = esp_mqtt_client_publish(mqtt_client, topic_temp, (char *)mqtt_data->data, mqtt_data->data_len, 0, 0);
				ESP_LOGI(TAG, "mqtt_task send %s msg_id %d data len %d", mqtt_data->topic,msg_id,mqtt_data->data_len);
				user_free(topic_temp);
			}else {
				ESP_LOGW(TAG, "mqtt_task not connect %s len %d",mqtt_data->topic,mqtt_data->data_len);
			}
			mqtt_data_free(mqtt_data);
		} else 
		{
			if(connected)
			{
				 char *msg = user_malloc(128);
				 sprintf(msg,"heartbeat %lu times",heartbeat_times);
				 heartbeat_times++;
				 blecent_cloud_upload_status(3,msg);
				 user_free(msg);
			}
		}
    }
}

void mqtt_init(void)
{	
	char *mqtt_url=(char *)user_malloc(50);
	device_config_get_string(CONFIG_DTU_MQTT_URL,mqtt_url, 0);
	uint32_t mqtt_port = device_config_get_number(CONFIG_DTU_MQTT_PORT);
	char *mqtt_user=(char *)user_malloc(50);
	device_config_get_string(CONFIG_DTU_MQTT_USER,mqtt_user, 0);	
	char *mqtt_password=(char *)user_malloc(50);
	device_config_get_string(CONFIG_DTU_MQTT_PASSWORD,mqtt_password, 0);	
	char *mqtt_hint_name=(char *)user_malloc(50);
	device_config_get_string(CONFIG_DTU_MQTT_HINT_NAME,mqtt_hint_name, 0);	
	char *mqtt_hint_key=(char *)user_malloc(50);
	device_config_get_string(CONFIG_DTU_MQTT_HINT_HEX_KEY,mqtt_hint_key, 0);	
    uint8_t *mqtt_hint_key_bytes=(uint8_t *)user_malloc(50);
    util_hex_to_bytes((const char *)mqtt_hint_key, mqtt_hint_key_bytes);
    mqtt_heartbeat = device_config_get_number(CONFIG_DTU_MQTT_HEARTBEAT_SECONDS);
    
    if(mqtt_url==NULL || strlen(mqtt_url)==0 || mqtt_port == 0)
    	return;
    
    #if 0
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_url,
        .broker.address.port = mqtt_port
        .broker.verification.psk_hint_key = &psk_hint_key,
        .credentials.username = mqtt_user,
        .credentials.authentication.password = mqtt_password
    };
    #endif 
	esp_mqtt_client_config_t mqtt_cfg = {0};
    mqtt_cfg.broker.address.uri = mqtt_url;
    mqtt_cfg.broker.address.port = mqtt_port;
    if(mqtt_hint_name!=NULL && strlen(mqtt_hint_name) > 1)
    {
		static psk_hint_key_t psk_hint_key = {0};
		psk_hint_key.key = mqtt_hint_key_bytes;
		size_t key_size = strlen(mqtt_hint_key)/2;
		memcpy((uint8_t *)&psk_hint_key.key_size,&key_size,sizeof(key_size));
		psk_hint_key.hint = mqtt_hint_name;
		mqtt_cfg.broker.verification.psk_hint_key = &psk_hint_key;
	}
	if(mqtt_user!=NULL && strlen(mqtt_user)>1 && mqtt_password!=NULL && strlen(mqtt_password) > 1)
	{
		mqtt_cfg.credentials.username = mqtt_user;
        mqtt_cfg.credentials.authentication.password = mqtt_password;
	}
    
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    g_queue_mqtt_upload = xQueueCreate(20, sizeof(mqtt_data_upload_t *));
    
	xTaskCreate(&mqtt_task, "mqtt_task", 10*1024, NULL, 9, NULL);
}


