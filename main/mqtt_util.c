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
#include "util.h"
#include "system_util.h"
#include "device_config.h"


static const char *TAG = "mqtts_util";

/*
 * Define psk key and hint as defined in mqtt broker
 * example for mosquitto server, content of psk_file:
 * hint:BAD123
 *
 */
static const uint8_t s_key[] = { 0xBA, 0xD1, 0x23 };

static const psk_hint_key_t psk_hint_key = {
        .key = s_key,
        .key_size = sizeof(s_key),
        .hint = "hint"
        };
static esp_mqtt_client_handle_t mqtt_client = NULL;        
static QueueHandle_t        g_queue_mqtt_send  = NULL;
static QueueHandle_t        g_queue_mqtt_recv  = NULL;
static bool connected = false;
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
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
        	connected = true;
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "/dc01/a085e3e66d1c/ble/a1/xxxyy", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
        	connected = false;
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            //ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("recv topic =%.*s data len %d\r\n", event->topic_len, event->topic,event->data_len);
            print_format(NULL,0,"%02x","recv data ", "\r\n",(uint8_t *)event->data,event->data_len);
            //printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

int mqtt_send(char *topic,uint8_t *data,uint32_t data_len) {
	char *topic_temp = user_malloc(strlen(topic)+32);
	sprintf(topic_temp,"/dc01/%02x%02x%02x%02x%02x%02x/%s",rt_device_mac[0],rt_device_mac[1],rt_device_mac[2],rt_device_mac[3],rt_device_mac[4],rt_device_mac[5],topic);
	ESP_LOGI(TAG, "mqtt_send %s data len %lu", topic_temp,data_len);
	int msg_id = esp_mqtt_client_publish(mqtt_client, topic_temp, (char *)data, data_len, 0, 0);
	ESP_LOGI(TAG, "sent publish msg_id=%d", msg_id);
	user_free(topic_temp);
	return 0;
}

static void mqtt_task(void *pvParameters)
{
	uint8_t test[10] ={1};
	
    while(1) 
    {
		if(connected) {
			mqtt_send("ble/a1/xxxyy",test,sizeof(test));
			test[3] = test[3]+1;
		}
		util_delay_ms(1000);
    }
}

void mqtt_init(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtts://139.196.107.200",
        .broker.address.port = 1883,
        .broker.verification.psk_hint_key = &psk_hint_key,
        .credentials.username = "user",
        .credentials.authentication.password ="quitto12vers"
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    //print_mem_info();
    g_queue_mqtt_send = xQueueCreate(10, sizeof(void *));
    g_queue_mqtt_recv = xQueueCreate(10, sizeof(void *));
    
	xTaskCreate(&mqtt_task, "mqtt_task", 10*1024, NULL, 9, NULL);
}


