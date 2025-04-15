#include "ch423.h"
#include "util.h"
#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include "usbh_modem_board.h"
#include "usbh_modem_wifi.h"
#include "network_util.h"
#include "dhcpserver/dhcpserver.h"
#include "lwip/lwip_napt.h"
#include "device_config.h"
#include "lwip/sockets.h"
#include <system_util.h>

static const char *TAG = "module_4g_util";

static void on_modem_event(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    if (event_base == MODEM_BOARD_EVENT) {
        if (event_id == MODEM_EVENT_SIMCARD_DISCONN) {
			rt_4g_ip=0;
	        rt_4g_mask=0;
	        rt_4g_gw=0;
            ESP_LOGW(TAG, "Modem Board Event: SIM Card disconnected");
            //led_indicator_start(s_led_system_handle, BLINK_CONNECTED);
        } else if (event_id == MODEM_EVENT_SIMCARD_CONN) {
            ESP_LOGI(TAG, "Modem Board Event: SIM Card Connected");
            //led_indicator_stop(s_led_system_handle, BLINK_CONNECTED);
        } else if (event_id == MODEM_EVENT_DTE_DISCONN) {
			rt_4g_ip=0;
	        rt_4g_mask=0;
	        rt_4g_gw=0;
            ESP_LOGW(TAG, "Modem Board Event: USB disconnected");
            //led_indicator_start(s_led_system_handle, BLINK_CONNECTING);
        } else if (event_id == MODEM_EVENT_DTE_CONN) {
            ESP_LOGI(TAG, "Modem Board Event: USB connected");
            //led_indicator_stop(s_led_system_handle, BLINK_CONNECTED);
            //led_indicator_stop(s_led_system_handle, BLINK_CONNECTING);
        } else if (event_id == MODEM_EVENT_DTE_RESTART) {
            ESP_LOGW(TAG, "Modem Board Event: Hardware restart");
            //led_indicator_start(s_led_system_handle, BLINK_CONNECTED);
        } else if (event_id == MODEM_EVENT_DTE_RESTART_DONE) {
            ESP_LOGI(TAG, "Modem Board Event: Hardware restart done");
            //led_indicator_stop(s_led_system_handle, BLINK_CONNECTED);
        } else if (event_id == MODEM_EVENT_NET_CONN) {
            ESP_LOGI(TAG, "Modem Board Event: Network connected");
            xEventGroupSetBits(app_evt_group_hdl, MODULE_4G_READY_BIT);
            //led_indicator_start(s_led_4g_handle, BLINK_CONNECTED);
        } else if (event_id == MODEM_EVENT_NET_DISCONN) {
			rt_4g_ip=0;
	        rt_4g_mask=0;
	        rt_4g_gw=0;
            ESP_LOGW(TAG, "Modem Board Event: Network disconnected");
            //led_indicator_stop(s_led_4g_handle, BLINK_CONNECTED);
        } else if (event_id == MODEM_EVENT_WIFI_STA_CONN) {
            ESP_LOGI(TAG, "Modem Board Event: Station connected");
            //led_indicator_start(s_led_wifi_handle, BLINK_CONNECTED);
        } else if (event_id == MODEM_EVENT_WIFI_STA_DISCONN) {
            ESP_LOGW(TAG, "Modem Board Event: All stations disconnected");
            //led_indicator_stop(s_led_wifi_handle, BLINK_CONNECTED);
        }
    }
}

static char rt_apn[50]={0};
void modem_board_call_back(uint8_t type,int count,...)
{
	va_list args;
    va_start(args, count);
	switch(type)
	{
		case 1://fetch imei ccid
			char *temp_buf=(char *)user_malloc(128);
		    esp_err_t (*func_imei)(void *, void *, void *) = va_arg(args, esp_err_t (*)(void *, void *, void *));
	    	esp_err_t (*func_iccid)(void *, void *, void *) = va_arg(args, esp_err_t (*)(void *, void *, void *));
	    	esp_err_t (*func_cgauth)(void *, void *, void *) = va_arg(args, esp_err_t (*)(void *, void *, void *));
	    	void *dce = va_arg(args, void *);
	    	esp_err_t err = func_imei(dce, NULL, temp_buf);
	    	if((err==ESP_OK)&&(strlen(temp_buf)>10)&&(strlen(temp_buf)<20))
			{
				bzero(rt_imei,sizeof(rt_imei));
				strcat(rt_imei,temp_buf);
			}
			bzero(temp_buf,128);
			err = func_iccid(dce, NULL, temp_buf);		
			if((err==ESP_OK)&&(strlen(temp_buf)>10)&&(strlen(temp_buf)<26))
			{
				bzero(rt_iccid,sizeof(rt_iccid));
				strcat(rt_iccid,temp_buf);
			}
			
			ESP_LOGI(TAG, "imei %s iccid %s",rt_imei,rt_iccid);
			if(device_config_get_number(CONFIG_4G_AUTH_NEED))
			{
				bzero(temp_buf,128);
				sprintf(temp_buf,"AT+CGAUTH=1,%lu,\"",device_config_get_number(CONFIG_4G_AUTH_TYPE));
				device_config_get_string(CONFIG_4G_AUTH_PASSWORD,temp_buf+strlen(temp_buf), 50);
				strcat(temp_buf,"\",\"");
				device_config_get_string(CONFIG_4G_AUTH_USER,temp_buf+strlen(temp_buf), 50);
				strcat(temp_buf,"\"\r");
				err = func_cgauth(dce, temp_buf, NULL);
			}
			user_free(temp_buf);
			break;
		case 2://reset 4g
			util_reset_4g(count);
			break;
		case 3://fetch 4g ip info v4
			uint8_t *ip = va_arg(args, uint8_t *);
			uint8_t *mask = va_arg(args, uint8_t *);
			uint8_t *gw = va_arg(args, uint8_t *);
			uint8_t *dns1 = va_arg(args, uint8_t *);
			uint8_t *dns2 = va_arg(args, uint8_t *);
			if(ip!=NULL)
				memcpy(&rt_4g_ip,ip,4);
			if(mask!=NULL)
				memcpy(&rt_4g_mask,mask,4);
			if(gw!=NULL)
				memcpy(&rt_4g_gw,gw,4);
			if(dns1!=NULL)
				memcpy(&rt_4g_dns1,dns1,4);
			if(dns2!=NULL)
				memcpy(&rt_4g_dns2,dns2,4);
			break;
		case 4://fetch 4g ip info v6
			uint8_t *ipv6 = va_arg(args, uint8_t *);
			if(ipv6!=NULL)
			{
				memcpy((uint8_t *)&rt_4g_ipv6[0],(uint8_t *)&(((esp_ip6_addr_t *)ipv6)->addr[0]),16);	
				memcpy(&rt_4g_ipv6_zone,(uint8_t *)&(((esp_ip6_addr_t *)ipv6)->zone),1);	
			}
			break;
		case 5://set apn
			bzero(rt_apn,sizeof(rt_apn));
			device_config_get_string(CONFIG_4G_APN,rt_apn, 50);
			if(strlen((char *)rt_apn)>1){
				char **temp_apn = va_arg(args, char **);
				*temp_apn=rt_apn;
			}
			break;
		case 6://csq
			rt_csq=count;
			break;
		default:
			break;
	}
	va_end(args);
}

int module_wifi_set_dns(void *ap_netif)
{
	 esp_netif_dns_info_t dns_new,dns_old;
	 struct sockaddr_in sa;
	 dhcps_offer_t dhcps_dns_value = OFFER_DNS;
	 
	 if((ap_netif==NULL)||((device_config_get_number(CONFIG_WIFI_MODE)&0x02)==0))
	 {
		ESP_LOGE(TAG, "module_wifi_set_dns no ap netif");
	 	return -1;
	 }
	 
	 dns_new.ip.u_addr.ip4.addr=0;
	 char * ip_temp=(char *)user_malloc(64);
	 device_config_get_string(CONFIG_WIFI_AP_DNS_DEFAULT,ip_temp, 0);
	 if((strlen(ip_temp)>5)&&(inet_pton(AF_INET, ip_temp, &(sa.sin_addr)) == 1))
	 {
        dns_new.ip.u_addr.ip4.addr = sa.sin_addr.s_addr;
	 }
	 user_free(ip_temp);
	 if(dns_new.ip.u_addr.ip4.addr==0)
	 {
		 esp_netif_t *netif=esp_netif_get_handle_from_ifkey("ETH_DEF");
		 if(netif==NULL)
		 	netif=esp_netif_get_handle_from_ifkey("PPP_DEF");
		 if(netif==NULL)
		 {
			ESP_LOGI(TAG, "module_wifi_set_dns no network");
		 	return -2;
		 }
		 esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_new);
     	 ESP_LOGI(TAG, "module_wifi_set_dns new main dns " IPSTR, IP2STR(&dns_new.ip.u_addr.ip4));
     	 esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_new);
     	 ESP_LOGI(TAG, "module_wifi_set_dns new backup dns " IPSTR, IP2STR(&dns_new.ip.u_addr.ip4));
	 }
 	 esp_netif_get_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns_old);
 	 ESP_LOGI(TAG, "module_wifi_set_dns old dns " IPSTR, IP2STR(&dns_old.ip.u_addr.ip4));

     if (dns_new.ip.u_addr.ip4.addr != dns_old.ip.u_addr.ip4.addr) 
     {
	    dns_new.ip.type = IPADDR_TYPE_V4;
	    esp_netif_dhcps_stop(ap_netif);
	    ESP_ERROR_CHECK(esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value)));
	    ESP_ERROR_CHECK(esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns_new));
	    esp_netif_dhcps_start(ap_netif);
	    rt_wifi_ap_dns=dns_new.ip.u_addr.ip4.addr;
        ESP_LOGI(TAG, "module_wifi_set_dns ap dns addr %s", inet_ntoa(dns_new.ip.u_addr.ip4.addr));
     }else {
		ESP_LOGI(TAG, "module_wifi_set_dns skip %s", inet_ntoa(dns_new.ip.u_addr.ip4.addr));
	 }
     return 0;
}
static esp_err_t module_wifi_napt_enable(bool enable)
{
    ip_napt_enable(_g_esp_netif_soft_ap_ip.ip.addr, enable);
    ESP_LOGI(TAG, "NAT is %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}
static int s_active_station_num = 0;
static void module_wifi_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data)
{
	 if(event_base == IP_EVENT)
	 {
		switch (event_id) {
			//ap
			case IP_EVENT_AP_STAIPASSIGNED:
			 	ESP_LOGI(TAG, "wifi assigned an ip");
				break;
			//sta
			case IP_EVENT_STA_GOT_IP:
			 	ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        		ESP_LOGI(TAG, "wifi got an ip " IPSTR, IP2STR(&event->ip_info.ip));
        		rt_wifi_sta_ip=event->ip_info.ip.addr;
        		rt_wifi_sta_mask=event->ip_info.netmask.addr;
        		rt_wifi_sta_gw=event->ip_info.gw.addr;
				break;
			case IP_EVENT_STA_LOST_IP:
				rt_wifi_sta_ip=0;
        		rt_wifi_sta_mask=0;
        		rt_wifi_sta_gw=0;
				break;
        default:
            break;
        }
	 }else if(event_base == WIFI_EVENT)
	 {
		switch (event_id) 
		{
			//ap
	        case WIFI_EVENT_AP_STACONNECTED:
	        	wifi_event_ap_staconnected_t *event1 = (wifi_event_ap_staconnected_t *) event_data;
	        	ESP_LOGI(TAG, "wifi station "MACSTR" joined, AID=%d",MAC2STR(event1->mac), event1->aid);
	            if (s_active_station_num == 0) {
	                module_wifi_napt_enable(true);
	            }
	            if (++s_active_station_num > 0) {
	                esp_event_post(MODEM_BOARD_EVENT, MODEM_EVENT_WIFI_STA_CONN, (void *)s_active_station_num, 0, 0);
	            }
	            rt_wifi_ap_sta_num++;
	            break;
	        case WIFI_EVENT_AP_STADISCONNECTED:
	         	wifi_event_ap_stadisconnected_t *event2 = (wifi_event_ap_stadisconnected_t *) event_data;
	        	ESP_LOGI(TAG, "wifi station "MACSTR" left, AID=%d", MAC2STR(event2->mac), event2->aid);
	            if (--s_active_station_num == 0) {
	                esp_event_post(MODEM_BOARD_EVENT, MODEM_EVENT_WIFI_STA_DISCONN, (void *)s_active_station_num, 0, 0);
	                //TODO: esp-lwip NAPT now have a bug, disable then enable NAPT will cause random table index
	                //module_wifi_napt_enable(false);
	            }
	            if(rt_wifi_ap_sta_num)
	           		rt_wifi_ap_sta_num--;
	            break;
	            
	        //sta
	        case WIFI_EVENT_STA_START:
	        	esp_wifi_connect();
	        	ESP_LOGI(TAG, "wifi station started");
	        	break;
	        default:
	            break;
        } 
	 }
}

void *module_wifi_start(void)
{
	uint8_t wifi_mode=device_config_get_number(CONFIG_WIFI_MODE);
	if((wifi_mode==0)||(wifi_mode>3)){
		ESP_LOGI(TAG, "module_wifi_start off %u",wifi_mode);
		return NULL;
	}
	wifi_mode_t wifi_mode_temp=0;
	wifi_config_t wifi_cfg = { 0 };
	if((wifi_mode&0x01)&&(wifi_mode&0x02))
		wifi_mode_temp=WIFI_MODE_APSTA;
	else if(wifi_mode&0x01)
		wifi_mode_temp=WIFI_MODE_STA;
	else if(wifi_mode&0x02)
		wifi_mode_temp=WIFI_MODE_AP;
	esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &module_wifi_event_handler, NULL,&instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &module_wifi_event_handler, NULL,&instance_got_ip));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode_temp));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	
    char *temp_buf=(char *)user_malloc(64);
    esp_netif_t **wifi_netif = user_malloc(sizeof(esp_netif_t*) * 2);
    
	if(wifi_mode&0x01)//sta
	{
		bzero(&wifi_cfg,sizeof(wifi_cfg));
		device_config_get_string(CONFIG_WIFI_AP_STA_SSID,temp_buf, 0);
    	strlcpy((char *)wifi_cfg.sta.ssid, temp_buf, 30);
    	bzero(temp_buf,64);
    	device_config_get_string(CONFIG_WIFI_AP_STA_PASSWORD,temp_buf, 0);
    	strlcpy((char *)wifi_cfg.sta.password, temp_buf, 12);
    	wifi_netif[0] = esp_netif_create_default_wifi_sta();
    	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    	ESP_LOGI(TAG, "module_wifi_start sta ssid %s|%d password %s|%d",(char *)wifi_cfg.sta.ssid,strlen((char *)wifi_cfg.sta.ssid),(char *)wifi_cfg.sta.password,strlen((char *)wifi_cfg.sta.password));
	}
    if(wifi_mode&0x02)//ap
	{
		bzero(&wifi_cfg,sizeof(wifi_cfg));
		device_config_get_string(CONFIG_WIFI_AP_STA_SSID,temp_buf, 0);
    	sprintf(temp_buf+strlen(temp_buf),"-%02x%02x%02x",rt_device_mac[3],rt_device_mac[4],rt_device_mac[5]);
    	strlcpy((char *)wifi_cfg.ap.ssid, temp_buf, 30);
    	bzero(temp_buf,64);
    	device_config_get_string(CONFIG_WIFI_AP_STA_PASSWORD,temp_buf, 0);
    	strlcpy((char *)wifi_cfg.ap.password, temp_buf, 12);
    	wifi_cfg.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;	
	    wifi_cfg.ap.max_connection = device_config_get_number(CONFIG_WIFI_AP_MAX_CONNECTION);	    
	    wifi_cfg.ap.channel = device_config_get_number(CONFIG_WIFI_AP_CHANNEL);
	    wifi_cfg.ap.ssid_hidden = device_config_get_number(CONFIG_WIFI_AP_SSID_HIDDEN);
	    wifi_netif[1] = esp_netif_create_default_wifi_ap();
	    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
	    ESP_LOGI(TAG, "module_wifi_start ap ssid %s|%d password %s|%d max connection %u channel %u ssid hidden %u",(char *)wifi_cfg.ap.ssid,strlen((char *)wifi_cfg.ap.ssid),(char *)wifi_cfg.ap.password,strlen((char *)wifi_cfg.ap.password),wifi_cfg.ap.max_connection,wifi_cfg.ap.channel,wifi_cfg.ap.ssid_hidden);
	}
    user_free(temp_buf);

    ESP_ERROR_CHECK(esp_wifi_start());
    if(wifi_mode&0x02)//ap
    {
		ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_BW_HT40));
		module_wifi_set_dns(wifi_netif[1]);
	}
	return wifi_netif;
}
void module_4g_start(uint32_t wait_seconds)
{
    modem_config_t modem_config = MODEM_DEFAULT_CONFIG();
    /* Modem init flag, used to control init process */
    /* if Not enter ppp, modem will enter command mode after init */
   // modem_config.flags |= MODEM_FLAGS_INIT_NOT_ENTER_PPP;
    /* if Not waiting for modem ready, just return after modem init */
    modem_config.flags |= MODEM_FLAGS_INIT_NOT_BLOCK;
    modem_config.handler = on_modem_event;
    modem_board_init(&modem_config);
    
    EventBits_t bits = xEventGroupWaitBits(app_evt_group_hdl, MODULE_4G_READY_BIT , pdFALSE, pdFALSE, 	wait_seconds*1000 / portTICK_PERIOD_MS);//wait some seconds for 4g module ready
	if( ( bits & MODULE_4G_READY_BIT ) != 0 )
		ESP_LOGI(TAG, "4g module is ready");
	else 
		ESP_LOGE(TAG, "4g module not work");
    
	//esp_netif_t *ppp_netif=esp_netif_get_handle_from_ifkey("PPP_DEF");
	//assert(ppp_netif != NULL);
	//int ppp_netif_index = esp_netif_get_netif_impl_index(ppp_netif);
}
