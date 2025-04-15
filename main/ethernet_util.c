/* Ethernet Basic Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "ethernet_util.h"
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "driver/spi_master.h"
#include "ch423.h"
#include "spi_util.h"
#include "device_config.h"
#include "system_util.h"
#include "util.h"

static const char *TAG = "ethernet_util";


typedef struct {
    uint8_t spi_cs_gpio;
    uint8_t int_gpio;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
    uint8_t *mac_addr;
}spi_eth_module_config_t;




/**
 * @brief Ethernet SPI modules initialization
 *
 * @param[in] spi_eth_module_config specific SPI Ethernet module configuration
 * @param[out] mac_out optionally returns Ethernet MAC object
 * @param[out] phy_out optionally returns Ethernet PHY object
 * @return
 *          - esp_eth_handle_t if init succeeded
 *          - NULL if init failed
 */

static esp_eth_handle_t eth_init_spi(spi_eth_module_config_t *spi_eth_module_config, esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
    esp_eth_handle_t ret = NULL;

    // Init common MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    // Update PHY config based on board specific configuration
    phy_config.phy_addr = spi_eth_module_config->phy_addr;
    phy_config.reset_gpio_num = spi_eth_module_config->phy_reset_gpio;

    // Configure SPI interface for specific SPI module
    
    spi_device_interface_config_t spi_devcfg = {
        .mode = 0,
        .clock_speed_hz = 5 * 1000 * 1000,
        .queue_size = 20,
        .spics_io_num = -1,
        .pre_cb=spi_cs_w5500_pre_cb, 
        .post_cb=spi_cs_w5500_post_cb
    };
    // Init vendor specific MAC config to default, and create new SPI Ethernet MAC instance
    // and new PHY instance based on board configuration

    //mac_config.cs_control=cs_control;
#if 0
    eth_dm9051_config_t dm9051_config = ETH_DM9051_DEFAULT_CONFIG(ETH_SPI_HOST, &spi_devcfg);
    dm9051_config.int_gpio_num = spi_eth_module_config->int_gpio;
    esp_eth_mac_t *mac = esp_eth_mac_new_dm9051(&dm9051_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_dm9051(&phy_config);
#endif
#if 1
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(SPI_HOST, &spi_devcfg);
    w5500_config.int_gpio_num = spi_eth_module_config->int_gpio;
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
#endif

    // Init Ethernet driver to default and install it
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_GOTO_ON_FALSE(esp_eth_driver_install(&eth_config_spi, &eth_handle) == ESP_OK, NULL, err, TAG, "SPI Ethernet driver install failed");

    // The SPI Ethernet module might not have a burned factory MAC address, we can set it manually.
    if (spi_eth_module_config->mac_addr != NULL) {
        ESP_GOTO_ON_FALSE(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, spi_eth_module_config->mac_addr) == ESP_OK,
                                        NULL, err, TAG, "SPI Ethernet MAC address config failed");
    }

    if (mac_out != NULL) {
        *mac_out = mac;
    }
    if (phy_out != NULL) {
        *phy_out = phy;
    }
    return eth_handle;
err:
    if (eth_handle != NULL) {
        esp_eth_driver_uninstall(eth_handle);
    }
    if (mac != NULL) {
        mac->del(mac);
    }
    if (phy != NULL) {
        phy->del(phy);
    }
    return ret;
}


static esp_err_t ethernet_init(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out)
{
    esp_err_t ret = ESP_OK;
    esp_eth_handle_t *eth_handles = NULL;
    uint8_t eth_cnt = 0;
    ESP_GOTO_ON_FALSE(eth_handles_out != NULL && eth_cnt_out != NULL, ESP_ERR_INVALID_ARG,err, TAG, "invalid arguments: initialized handles array or number of interfaces");
    eth_handles = calloc(1, sizeof(esp_eth_handle_t));
    ESP_GOTO_ON_FALSE(eth_handles != NULL, ESP_ERR_NO_MEM, err, TAG, "no memory");
    // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
    spi_eth_module_config_t spi_eth_module_config[1] = { 0 };
    spi_eth_module_config[0].spi_cs_gpio = 0;
    spi_eth_module_config[0].int_gpio = ETH_INT;
    spi_eth_module_config[0].phy_reset_gpio = -1;
    spi_eth_module_config[0].phy_addr = 1;
    // The SPI Ethernet module(s) might not have a burned factory MAC address, hence use manually configured address(es).
    // In this example, Locally Administered MAC address derived from ESP32x base MAC address is used.
    // Note that Locally Administered OUI range should be used only when testing on a LAN under your control!
    uint8_t base_mac_addr[ETH_ADDR_LEN];
    ESP_GOTO_ON_ERROR(esp_efuse_mac_get_default(base_mac_addr), err, TAG, "get EFUSE MAC failed");
    uint8_t local_mac_1[ETH_ADDR_LEN];
    esp_derive_local_mac(local_mac_1, base_mac_addr);
    spi_eth_module_config[0].mac_addr = local_mac_1;
    eth_handles[eth_cnt] = eth_init_spi(&spi_eth_module_config[0], NULL, NULL);
    ESP_GOTO_ON_FALSE(eth_handles[eth_cnt], ESP_FAIL, err, TAG, "SPI Ethernet init failed");
    eth_cnt++;
    *eth_handles_out = eth_handles;
    *eth_cnt_out = eth_cnt;
    return ret;
err:
    free(eth_handles);
    return ret;
}
/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        rt_w5500_sta_ip=0;
		rt_w5500_sta_mask=0;
		rt_w5500_sta_gw=0;
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        //xEventGroupSetBits(app_evt_group_hdl, MODULE_ETH_READY_BIT);
        break;
    case ETHERNET_EVENT_STOP:
        rt_w5500_sta_ip=0;
		rt_w5500_sta_mask=0;
		rt_w5500_sta_gw=0;
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
	esp_netif_t *netif = event->esp_netif;
	esp_netif_dns_info_t dns_info;
    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
    ESP_LOGI(TAG, "Main DNS: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    rt_w5500_sta_dns1=dns_info.ip.u_addr.ip4.addr;
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
    ESP_LOGI(TAG, "Backup DNS: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    rt_w5500_sta_dns2=dns_info.ip.u_addr.ip4.addr;                    
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    
    rt_w5500_sta_ip=ip_info->ip.addr;
	rt_w5500_sta_mask=ip_info->netmask.addr;
	rt_w5500_sta_gw=ip_info->gw.addr;
    //xEventGroupSetBits(app_evt_group_hdl, MODULE_ETH_READY_BIT);
}
static void got_ipv6_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    const esp_netif_ip6_info_t *ip6_info = &event->ip6_info;
    if(strncmp("eth", esp_netif_get_desc(event->esp_netif), 3) != 0)
    {
		ESP_LOGW(TAG, "ethernet_util ipv6 from another netif: ignored %s",esp_netif_get_desc(event->esp_netif));
		return;
	}
    ESP_LOGI(TAG, "ethernet_util ipv6 interface %s desc %s",esp_netif_get_ifkey(event->esp_netif),esp_netif_get_desc(event->esp_netif));
    //esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&ip6_info->ip);
    ESP_LOGI(TAG, "ethernet_util ipv6 " IPV6STR "", IPV62STR(ip6_info->ip));
    
    memcpy((uint8_t *)&rt_w5500_sta_ipv6[0],(uint8_t *)&(((esp_ip6_addr_t *)&ip6_info->ip)->addr[0]),16);	
	memcpy(&rt_w5500_sta_ipv6_zone,(uint8_t *)&(((esp_ip6_addr_t *)&ip6_info->ip)->zone),1);	
				
}
static esp_err_t util_netif_set_dns(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
{
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}
static void ethernet_static_ip(esp_netif_t *netif)
{
	if (esp_netif_dhcpc_stop(netif) != ESP_OK) {
        ESP_LOGE(TAG, "util_netif_static_ip failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , sizeof(esp_netif_ip_info_t));
    char * ip_temp=(char *)user_malloc(64);
	device_config_get_string(CONFIG_W5500_IP,ip_temp, 0);
	ESP_LOGI(TAG, "util_netif_static_ip ip=%s",ip_temp);
    ip.ip.addr = ipaddr_addr(ip_temp);
    bzero(ip_temp,64);
    device_config_get_string(CONFIG_W5500_SUBNET_MASK,ip_temp, 0);
    ESP_LOGI(TAG, "util_netif_static_ip netmask=%s",ip_temp);
    ip.netmask.addr = ipaddr_addr(ip_temp);
    bzero(ip_temp,64);
    device_config_get_string(CONFIG_W5500_GATEWAY,ip_temp, 0);
    ESP_LOGI(TAG, "util_netif_static_ip gateway=%s",ip_temp);
    ip.gw.addr = ipaddr_addr(ip_temp);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
		user_free(ip_temp);
        ESP_LOGE(TAG, "util_netif_static_ip failed to set ip info");
        return;
    }
    ESP_LOGI(TAG, "util_netif_static_ip set static ip success");
    bzero(ip_temp,64);
    device_config_get_string(CONFIG_W5500_DNS1,ip_temp, 0);
    ESP_LOGI(TAG, "util_netif_static_ip dns main=%s",ip_temp);
    ESP_ERROR_CHECK(util_netif_set_dns(netif, ipaddr_addr(ip_temp), ESP_NETIF_DNS_MAIN));
    bzero(ip_temp,64);
    device_config_get_string(CONFIG_W5500_DNS2,ip_temp, 0);
    ESP_LOGI(TAG, "util_netif_static_ip dns back=%s",ip_temp);
    ESP_ERROR_CHECK(util_netif_set_dns(netif, ipaddr_addr(ip_temp), ESP_NETIF_DNS_BACKUP));
    user_free(ip_temp);
    ESP_LOGI(TAG, "util_netif_static_ip set static dns success");
}

void ethernet_start(void)
{
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    if(ethernet_init(&eth_handles, &eth_port_cnt)!=ESP_OK)
    {
		ESP_LOGE(TAG, "ethernet start failed");
    	return;
    }
    ESP_LOGI(TAG, "eth_port_cnt %u",eth_port_cnt);
	esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
	esp_netif_t *eth_netif = esp_netif_new(&cfg);
	ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handles[0])));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &got_ipv6_event_handler, NULL));
	if(device_config_get_number(CONFIG_W5500_MODE)==2)
		ethernet_static_ip(eth_netif);

    ESP_ERROR_CHECK(esp_eth_start(eth_handles[0]));
}
uint8_t ethernet_check(void)
{
	spi_device_interface_config_t devcfg = { .command_bits = 16,
			.address_bits = 8, .dummy_bits = 0,
			.clock_speed_hz =  5 * 1000 * 1000,
			.duty_cycle_pos = 128, //50% duty cycle
			.mode = 0,
	        .spics_io_num = -1,
			.cs_ena_posttrans = 3, //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
			.queue_size = 20 };

	uint8_t version=0,i=0;
	for(;i<5;i++)
	{
		power_onoff_eth(1);
		util_delay_ms(100);
		spiutil_init(SPI_W5500, "W5500",&devcfg);
		util_delay_ms(100);
		spiutil_w5500_read(0x0039,&version,1);
		ESP_LOGI(TAG, "w5500 version %u",version);
		if(version==4)
		{
			ESP_LOGI(TAG, "ethernet_valid success");
			spiutil_deinit(SPI_W5500);
			return 1;
		}
		else
		{
			spiutil_deinit(SPI_W5500);
			ESP_LOGI(TAG, "w5500 restart %u",i);
			util_delay_ms(100);
			power_onoff_eth(0);
			util_delay_ms(500);
		}
	}
	return 0;
}
