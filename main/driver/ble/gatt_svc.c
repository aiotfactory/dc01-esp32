/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "gatt_svc.h"
#include "common.h"
#include "gap.h"
#include "util.h"
#include "system_util.h"
#include "device_config.h"


static const char *TAG = "ble_gatt_svc";

/* Private function declarations */
static int config_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);


/* 自定义服务 UUID: 0000FFF0-0000-1000-8000-00805F9B34FB */
static const ble_uuid128_t config_svc_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0xF0, 0xFF,
                     0x00, 0x00, 0x10, 0x00,
                     0x80, 0x00, 0x00, 0x80,
                     0x5F, 0x9B, 0x34, 0xFB);

/* 自定义特征 UUIDs */
static const ble_uuid128_t value_chr_uuid =
    BLE_UUID128_INIT(0x00, 0x00, 0xF2, 0xFF,
                     0x00, 0x00, 0x10, 0x00,
                     0x80, 0x00, 0x00, 0x80,
                     0x5F, 0x9B, 0x34, 0xFB);
static uint16_t value_chr_val_handle;
static uint16_t value_chr_attr_handle; 

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &config_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {/* Value characteristic */
              .uuid = &value_chr_uuid.u,
              .access_cb = config_access,
              #if SECURITY_ONOFF
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC | BLE_GATT_CHR_F_NOTIFY,
              #else
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
              #endif
              .val_handle = &value_chr_val_handle
              },
             {
                 0, /* No more characteristics in this service. */
             }}},
    {
        0, /* No more services. */
    },
};
static void send_notification(uint16_t conn_handle,char *msg) {
    struct os_mbuf *om;

    // 准备要发送的通知数据
    om = ble_hs_mbuf_from_flat(msg, strlen(msg));
    if (om != NULL && is_connection_encrypted(conn_handle)) {
        // 发送通知
        ble_gatts_notify_custom(conn_handle, value_chr_val_handle, om);
    }
}
static const char *help_msg={"type 1 to read, 1#xxx to config, rt to reboot"};
static int config_access(uint16_t conn_handle, uint16_t attr_handle,
                         struct ble_gatt_access_ctxt *ctxt, void *arg) {
    switch (ctxt->chr->uuid->type) {
        case BLE_UUID_TYPE_128:
            if (ble_uuid_cmp(ctxt->chr->uuid, &value_chr_uuid.u) == 0) {
				value_chr_attr_handle=attr_handle;
                // Handle read/write for value characteristic
                if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
                    return os_mbuf_append(ctxt->om, help_msg, strlen(help_msg));
                } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
                    // Process the new value from ctxt->om
                    size_t len = OS_MBUF_PKTLEN(ctxt->om);
                    uint8_t buf[len];
                    os_mbuf_copydata(ctxt->om, 0, len, buf);
                    // Handle the written data
                    ESP_LOGI("GATT", "Received data: %d %.*s", len,(int)len, buf);
                    if((len>=2)&&(memcmp(buf,"rt",2)==0))
                    {
						send_notification(conn_handle,"rebooting...");
						util_reboot(20);
						return 0;
					}
					uint8_t *sign=byte_search(buf, len,'#',1);
					char notif_msg[50]={0};
					int idx=-1;
					if((sign==NULL)&&(len<5)) //read config
					{
						idx = atoi((char *)buf);
						if((idx>0)&&(idx<CONFIG_NAME_MAX))
						{
							device_config_get_string(idx,notif_msg, sizeof(notif_msg));
							send_notification(conn_handle,notif_msg);
							return 0;
						}
						send_notification(conn_handle,"wrong idx");
						return 0;
					}
					if((sign!=NULL)&&((sign-buf)<5)) //write config
					{
						sign[0]=0;
						idx = atoi((char *)buf);
						if((idx>0)||(idx<CONFIG_NAME_MAX))
						{
							uint8_t ret=device_config_set_binary(idx,sign+1,len-((sign-buf)+1));
							if(ret==0) sprintf(notif_msg,"same as before");
							else if(ret==2) sprintf(notif_msg,"invalid value");
							else {config_write();sprintf(notif_msg,"write ok");}
							send_notification(conn_handle,notif_msg);
							return 0;
						}
						send_notification(conn_handle,"wrong idx");
						return 0;
					}
					send_notification(conn_handle,"wrong info");
                }
            }
            break;
        default:
            break;
    }
    return 0;
}


/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

int gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check attribute handle */
    if (event->subscribe.attr_handle == value_chr_attr_handle) {
        /* Check security status */
        if (!is_connection_encrypted(event->subscribe.conn_handle)) {
            ESP_LOGE(TAG, "failed to subscribe to heart rate measurement, "
                          "connection not encrypted!");
            return BLE_ATT_ERR_INSUFFICIENT_AUTHEN;
        }
    }
    return 0;
}


/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
