/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "ble_central_util.h"
#include "cJSON.h"
#include "util.h"
#include "system_util.h"


struct ble_find_condition {
    int id;
    int adv_prefix_start;
    int adv_prefix_len;
    uint8_t adv_prefix_value[32];
    char mac[13];
    int mac_len;
    char service[64];
    char character[64];
};

struct ble_info {
	struct ble_find_condition condition;
	int rssi;
	char mac[13];
};

static const char *TAG = "ble_central_util";
#define MAX_ADV_INFOS 50
static struct ble_find_condition g_adv_list[MAX_ADV_INFOS];
static int g_adv_count = 0;
static SemaphoreHandle_t g_adv_mutex = NULL;



static int blecent_gap_event(struct ble_gap_event *event, void *arg);
static int blecent_read(const struct peer *peer,struct ble_info *info);
static void blecent_scan(void);

void ble_store_config_init(void);


// 将标准格式的 128-bit UUID 字符串（带连字符）转换为 ble_uuid128_t
// 返回 0 成功，-1 失败
int str_to_uid128(const char *str, ble_uuid128_t *uuid128) {
    if (!str || !uuid128) return -1;
    char hex[33] = {0};
    int j = 0;
    for (int i = 0; str[i] && j < 32; i++) {
        if (str[i] == '-') continue;
        if (!isxdigit((unsigned char)str[i])) return -1;
        hex[j++] = str[i];
    }
    if (j != 32) return -1;
    for (int i = 0; i < 16; i++) {
        char byte_str[3] = {hex[30 - i*2], hex[31 - i*2], '\0'}; // 注意：反向读取
        uuid128->value[i] = (uint8_t)strtoul(byte_str, NULL, 16);
    }
    uuid128->u.type = BLE_UUID_TYPE_128;
    return 0;
}

static int hex_to_uint8(const char *hex_str,  uint8_t *out) {
	if (!hex_str || !out)
		return -1;
	size_t len = strlen(hex_str);

    for (size_t i = 0; i < len; i += 2) {
        char c1 = hex_str[i];
        char c2 = hex_str[i + 1];

        // 检查是否为合法十六进制字符
        if (!isxdigit((unsigned char)c1) || !isxdigit((unsigned char)c2)) {
            return -1;
        }

        // 转换高4位
        uint8_t val1 = (c1 >= 'A') ? (c1 & 0xDF) - 'A' + 10 : c1 - '0';
        // 转换低4位
        uint8_t val2 = (c2 >= 'A') ? (c2 & 0xDF) - 'A' + 10 : c2 - '0';

        out[i / 2] = (val1 << 4) | val2;
    }
    return 0;
}
static char *uint8_to_hex(const uint8_t *bin, int bin_len) {
    if (!bin || bin_len <= 0) return NULL;
    char *hex = user_malloc(bin_len * 2 + 1);
    for (int i = 0; i < bin_len; i++) {
        sprintf(&hex[i * 2], "%02x", bin[i]);
    }
    return hex;
}
/*
{
  "id": 3,
  "adv_prefix_start": 5,
  "adv_prefix_value": "DC02",
  "mac": "112233445566",
  "service": "FB349B5F-8000-0080-0010-0000FFF00000",
  "character": "FB349B5F-8000-0080-0010-0000FFF20000"
}
*/
static int adv_info_parse(const char *input_str, struct ble_find_condition *out_info) {
    if (out_info == NULL || input_str == NULL) return 0;

    cJSON *root = cJSON_Parse(input_str);
    if (root == NULL) return 0;

    int success = 1;

    cJSON *item;

    item = cJSON_GetObjectItem(root, "id");
    if (!cJSON_IsNumber(item)) { success = 0; } else { out_info->id = item->valueint; }

	item = cJSON_GetObjectItem(root, "adv_prefix_value");
    if (cJSON_IsString(item)) 
    { 
		out_info->adv_prefix_len = strlen(item->valuestring)/2;
		memset(out_info->adv_prefix_value,0,sizeof(out_info->adv_prefix_value));
		hex_to_uint8(item->valuestring,  out_info->adv_prefix_value);
		item = cJSON_GetObjectItem(root, "adv_prefix_start");
	    if (cJSON_IsNumber(item)) {
			out_info->adv_prefix_start = item->valueint; 
		} else {
 			out_info->adv_prefix_start=0;
 		}
	}
	
    item = cJSON_GetObjectItem(root, "mac");
    if (cJSON_IsString(item))  
    { 
		strncpy(out_info->mac, item->valuestring, sizeof(out_info->mac) - 1); 
		out_info->mac[sizeof(out_info->mac) - 1] = '\0'; 
		out_info->mac_len = strlen(item->valuestring);
		out_info->mac_len = out_info->mac_len>12?12:out_info->mac_len;
	}

    item = cJSON_GetObjectItem(root, "service");
    if (cJSON_IsString(item))
    { 
		strncpy(out_info->service, item->valuestring, sizeof(out_info->service) - 1); 
		out_info->service[sizeof(out_info->service) - 1] = '\0'; 
	}

    item = cJSON_GetObjectItem(root, "character");
    if (cJSON_IsString(item))
	{ 
		strncpy(out_info->character, item->valuestring, sizeof(out_info->character) - 1); 
		out_info->character[sizeof(out_info->character) - 1] = '\0'; 
	}
	if(out_info->mac_len==0 && (out_info->service[0]==0||out_info->character[0]==0))
		success = 0;
		
    cJSON_Delete(root);

    return success ? 1 : 0;
}
static bool adv_info_search(const uint8_t *mac_6bytes, const uint8_t *adv, int adv_len, struct ble_info *info) {
    static char mac_temp[13];
    bool ret = false;
    memset(mac_temp,0,sizeof(mac_temp));
    sprintf(mac_temp,"%02x%02x%02x%02x%02x%02x",mac_6bytes[0],mac_6bytes[1],mac_6bytes[2],mac_6bytes[3],mac_6bytes[4],mac_6bytes[5]);
    if (xSemaphoreTake(g_adv_mutex, portMAX_DELAY) == pdTRUE) 
    {
	    for (int i = 0; i < g_adv_count; i++) 
	    {
			#if 1
			if(adv_len>8 && memcmp(adv+5,"DC02",4)==0)
			{
				printf("watching device prefix len %d prefix start %d mac %s adv %s \r\n",g_adv_list[i].adv_prefix_len,g_adv_list[i].adv_prefix_start,mac_temp,adv);
				print_format(NULL,0,"%02x","adv ", "\r\n",adv + g_adv_list[i].adv_prefix_start,g_adv_list[i].adv_prefix_len);
				print_format(NULL,0,"%02x","info ", "\r\n",g_adv_list[i].adv_prefix_value,g_adv_list[i].adv_prefix_len);
			} 
			#endif
	        int mac_ok = (g_adv_list[i].mac_len == 0) || (memcmp(mac_temp, g_adv_list[i].mac,g_adv_list[i].mac_len) == 0);
	        int adv_ok = 0;
	
	        if (g_adv_list[i].adv_prefix_len == 0) {
	            adv_ok = 1;
	        } else {
	            if (g_adv_list[i].adv_prefix_len + g_adv_list[i].adv_prefix_start <= adv_len) {
	                adv_ok = (memcmp(g_adv_list[i].adv_prefix_value, adv + g_adv_list[i].adv_prefix_start, g_adv_list[i].adv_prefix_len) == 0);
	            }
	        }
	
	        if (mac_ok > 0 && adv_ok > 0 ) {
	            memcpy(&info->condition,&g_adv_list[i],sizeof(struct ble_find_condition));
	            memcpy(info->mac,mac_temp,12);
	            ret = true;
	            break;
	        }
	    }
    	xSemaphoreGive(g_adv_mutex);
   	}
    
    return ret;
}
static int adv_info_compare(const struct ble_find_condition *a, const struct ble_find_condition *b) {
    if (a == NULL || b == NULL) return 0;
    return a->id == b->id;
}
static void adv_info_add(const struct ble_find_condition *new_info) {
    if (new_info == NULL) return;
    if (xSemaphoreTake(g_adv_mutex, portMAX_DELAY) == pdTRUE) {
        int duplicate = 0;
        for (int i = 0; i < g_adv_count; i++) {
            if (adv_info_compare(&g_adv_list[i], new_info)) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate && g_adv_count < MAX_ADV_INFOS) {
            g_adv_list[g_adv_count++] = *new_info;
        }
        xSemaphoreGive(g_adv_mutex);
    }
}

static int adv_info_remove(int id) {
    if (xSemaphoreTake(g_adv_mutex, portMAX_DELAY) == pdTRUE) {
        int found = -1;
        for (int i = 0; i < g_adv_count; i++) {
            if (g_adv_list[i].id == id) {
                found = i;
                break;
            }
        }
        if (found >= 0) {
            int num_to_move = g_adv_count - found - 1;
            if (num_to_move > 0) {
                memcpy(&g_adv_list[found], &g_adv_list[found + 1], num_to_move * sizeof(struct ble_find_condition));
            }
            g_adv_count--;
        }
        xSemaphoreGive(g_adv_mutex);
        return found >= 0 ? 0 : -1;
    }
    return -1;
}
void blecent_device_read(char *command) {
	/*
	topic /dc01/a085e3e66d1c/ble/read
	*/
	const char command_json[] = "{"
	    "\"id\": 3,"
	    "\"adv_prefix_start\": 5,"
	    "\"adv_prefix_value\": \"44433032\"," //DC02
	    "\"mac\": \"2fa1651de6ff\","
	    "\"service\": \"fb349b5f-8000-0080-0010-0000fff00000\","
	    "\"character\": \"fb349b5f-8000-0080-0010-0000fff20000\""
	"}";
	struct ble_find_condition adv = {0};
	if(adv_info_parse(command_json, &adv)) {
		adv_info_add(&adv);
	}
}
void blecent_cloud_upload(int id, char *mac, int rssi, uint8_t *bin, int bin_len) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", id);
    cJSON_AddStringToObject(root, "mac", mac ? mac : "");
    cJSON_AddNumberToObject(root, "rssi", rssi);
    char *hex_str = uint8_to_hex(bin, bin_len);
    if (hex_str) {
        cJSON_AddStringToObject(root, "data", hex_str);
        user_free(hex_str); 
    } 
    char *json_str = cJSON_PrintUnformatted(root); // 紧凑格式，无空格
    if (json_str) {
        printf("blecent_cloud_upload %s\n", json_str);
        user_free(json_str);
    }
    cJSON_Delete(root);
}
static int
blecent_read_cb(uint16_t conn_handle,
                       const struct ble_gatt_error *error,
                       struct ble_gatt_attr *attr,
                       void *arg)
{
    ESP_LOGI(TAG,"Read complete for the subscribable characteristic status=%d conn_handle=%d", error->status, conn_handle);
    if (error->status == 0) {
        ESP_LOGI(TAG, " attr_handle=%d value=", attr->handle);
        //print_mbuf(attr->om);
        struct ble_info *info=(struct ble_info *)arg;
        uint16_t bin_len = OS_MBUF_PKTLEN(attr->om);
        uint8_t *bin = user_malloc(bin_len);  
	    os_mbuf_copydata(attr->om, 0, bin_len, bin);
        blecent_cloud_upload(info->condition.id, info->mac, info->rssi, bin, bin_len);
        user_free(bin);
    }
    ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    user_free(arg);
    return 0;
}
static int blecent_read(const struct peer *peer,struct ble_info *info)
{
    const struct peer_chr *chr;
    int rc;
    if(info!=NULL && info->condition.service[0]>0 && info->condition.character[0]>0) 
    {
	    ble_uuid128_t service = {0};
	    str_to_uid128(info->condition.service, &service);
	    ble_uuid128_t character = {0};
	    str_to_uid128(info->condition.character, &character);
	    #if 0
	    printf("input service %s character %s\r\n",info->condition.service,info->condition.character);
	    printf("converted service and character\r\n");
	    print_uuid(&service);
	    print_uuid(&character);
	    #endif 
	    chr = peer_chr_find_uuid(peer, (ble_uuid_t *)&service, (ble_uuid_t *)&character);
	    if (chr == NULL) {
	        ESP_LOGE(TAG,"Error: Peer doesn't have the custom subscribable characteristic");
	        goto err;
	    }
	    rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle, blecent_read_cb, info);
	    if (rc != 0) {
	        ESP_LOGE(TAG,"Error: Failed to read the custom subscribable characteristic rc=%d", rc);
	        goto err;
	    }
	}
    return 0;
err:
    user_free(info);
    return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

/**
 * Called when service discovery of the specified peer has completed.
 */
static void
blecent_disc_cb(const struct peer *peer, int status, void *arg)
{
    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        ESP_LOGE(TAG, "Error: Service discovery failed; status=%d conn_handle=%d", status, peer->conn_handle);
        user_free(arg);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }
    
	blecent_read(peer,(struct ble_info *)arg);
}

/**
 * Initiates the GAP general discovery procedure.
 */
static void
blecent_scan(void)
{
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int rc;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device.
     */
    disc_params.filter_duplicates = 0;

    /**
     * Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser.
     */
    disc_params.passive = 1;

    /* Use defaults for the rest of the parameters. */
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                      blecent_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error initiating GAP discovery procedure; rc=%d\n",
                    rc);
    }
}


static int
blecent_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;
	uint8_t own_addr_type;
	ble_addr_t *addr;
	
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
		/* Figure out address to use for connect (no privacy for now) */
	    rc = ble_hs_id_infer_auto(0, &own_addr_type);
	    if (rc != 0) {
			ESP_LOGE(TAG, "Error determining address type; rc=%d\n", rc); 
	        return 0; 
	    } 
	    addr = &((struct ble_gap_disc_desc *)&event->disc)->addr;
	    struct ble_info *adv_temp = user_malloc(sizeof(struct ble_info));
		if(adv_info_search(addr->val, event->disc.data, event->disc.length_data, adv_temp))
		{                    
    		printf("Find ble device %d %d %s\r\n",event->disc.event_type,event->disc.length_data,adv_temp->mac);
    		adv_temp->rssi = ((struct ble_gap_disc_desc *)&event->disc)->rssi;
        	if (event->disc.event_type == BLE_HCI_ADV_RPT_EVTYPE_ADV_IND || event->disc.event_type == BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
	   	        if(ble_gap_disc_cancel()==0) {
			    	if(ble_gap_connect(own_addr_type, addr, 30000, NULL, blecent_gap_event, adv_temp)==0) {
						return 0;
				    }
				}
			}
	    }
	    user_free(adv_temp);
        return 0;

    case BLE_GAP_EVENT_LINK_ESTAB:
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            //print_conn_desc(&desc);
            rc = peer_add(event->connect.conn_handle);
            if (rc != 0) {
                ESP_LOGE(TAG,"Failed to add peer; rc=%d", rc);
                user_free(arg);
                return 0;
            }


#if CONFIG_EXAMPLE_ENCRYPTION
            /** Initiate security - It will perform
             * Pairing (Exchange keys)
             * Bonding (Store keys)
             * Encryption (Enable encryption)
             * Will invoke event BLE_GAP_EVENT_ENC_CHANGE
             **/
            rc = ble_gap_security_initiate(event->connect.conn_handle);
            if (rc != 0) {
                ESP_LOGI(TAG, "Security could not be initiated, rc = %d\n", rc);
                return ble_gap_terminate(event->connect.conn_handle,
                                         BLE_ERR_REM_USER_CONN_TERM);
            } else {
                ESP_LOGI(TAG, "Connection secured\n");
            }
#else
            /* Perform service discovery */
            rc = peer_disc_all(event->connect.conn_handle, blecent_disc_cb, arg);
            if(rc != 0) {
                ESP_LOGE(TAG, "Failed to discover services; rc=%d", rc);
                user_free(arg);
                return 0;
            }
#endif
        } else {
            /* Connection attempt failed; resume scanning. */
            ESP_LOGE(TAG, "Error: Connection failed; status=%d", event->connect.status);
            user_free(arg);
            blecent_scan();
        }

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Connection terminated. */
        //ESP_LOGI(TAG, "disconnect; reason=%d ", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);
        ESP_LOGI(TAG, "\n");

        /* Forget about peer. */
        peer_delete(event->disconnect.conn.conn_handle);

        /* Resume scanning. */
        blecent_scan();
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        //ESP_LOGI(TAG, "discovery complete; reason=%d\n", event->disc_complete.reason);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        ESP_LOGI(TAG, "encryption change event; status=%d ",event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        print_conn_desc(&desc);
#if CONFIG_EXAMPLE_ENCRYPTION
        /*** Go for service discovery after encryption has been successfully enabled ***/
        rc = peer_disc_all(event->connect.conn_handle,
                           blecent_disc_cb, NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to discover services; rc=%d\n", rc);
            return 0;
        }
#endif
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        /* Peer sent us a notification or indication. */
        ESP_LOGI(TAG, "received %s; conn_handle=%d attr_handle=%d "
                    "attr_len=%d\n",
                    event->notify_rx.indication ?
                    "indication" :
                    "notification",
                    event->notify_rx.conn_handle,
                    event->notify_rx.attr_handle,
                    OS_MBUF_PKTLEN(event->notify_rx.om));

        /* Attribute data is contained in event->notify_rx.om. Use
         * `os_mbuf_copydata` to copy the data received in notification mbuf */
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    default:
        return 0;
    }
}

static void
blecent_reset_cb(int reason)
{
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

static void
blecent_sync_cb(void)
{
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    blecent_scan();
}

static void blecent_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
} 


void blecent_init(void)
{
	g_adv_mutex = xSemaphoreCreateMutex();
	
	blecent_device_read(NULL);
	
    int ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }

    /* Configure the host. */
    ble_hs_cfg.reset_cb = blecent_reset_cb;
    ble_hs_cfg.sync_cb = blecent_sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Initialize data structures to track connected peers. */
    int rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);
    assert(rc == 0);

    /* Set the default device name. */
    char *ble_name = user_malloc(32);
    sprintf(ble_name,"DC01%02X%02X%02X%02X%02X%02X",rt_device_mac[0],rt_device_mac[1],rt_device_mac[2],rt_device_mac[3],rt_device_mac[4],rt_device_mac[5]);
    rc = ble_svc_gap_device_name_set(ble_name);
    user_free(ble_name);
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    nimble_port_freertos_init(blecent_host_task);
}
