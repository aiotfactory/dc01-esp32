#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include <esp_err.h>
#include <esp_event.h>
#include <esp_event_base.h>
#include "esp32s3/rom/md5_hash.h"
#include "device_config.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * 设备类型，常量，用于标识设备的具体类型。
 * Device type, a constant used to identify the specific type of device.
 */
extern const uint32_t DEVICE_TYPE;

/**
 * 设备固件版本号，表示当前设备上运行的固件版本。
 * Device firmware version, indicating the current firmware version running on the device.
 */
extern uint32_t DEVICE_FIRMWARE_VERSION;

/**
 * 设备的MAC地址，用于网络通信中的唯一标识符。
 * The MAC address of the device, used as a unique identifier in network communication.
 */
extern uint8_t rt_device_mac[6];

/**
 * 当前信号质量，表示设备接收到的信号强度或质量。
 * Current signal quality, representing the signal strength or quality received by the device.
 */
extern uint8_t rt_csq;

/**
 * 设备的IMEI号码，用于移动设备的唯一标识。
 * The IMEI number of the device, used as a unique identifier for mobile devices.
 */
extern char rt_imei[30];

/**
 * 设备的ICCID号码，用于SIM卡的唯一标识。
 * The ICCID number of the device, used as a unique identifier for SIM cards.
 */
extern char rt_iccid[30];

/**
 * 4G网络相关的IP地址、子网掩码、网关、DNS服务器等配置信息。
 * 4G network related configuration information such as IP address, subnet mask, gateway, DNS servers, etc.
 */
extern uint32_t rt_4g_ip, rt_4g_mask, rt_4g_gw, rt_4g_dns1, rt_4g_dns2, rt_4g_ipv6_zone;
extern uint32_t rt_4g_ipv6[4], rt_w5500_sta_ipv6[4];

/**
 * Wi-Fi STA模式下的IP地址、子网掩码、网关、DNS服务器以及AP模式下的DNS服务器和STA连接数。
 * Configuration information for Wi-Fi STA mode including IP address, subnet mask, gateway, DNS servers, 
 * and for AP mode including DNS server and STA connection count.
 */
extern uint32_t rt_wifi_sta_ip, rt_wifi_sta_mask, rt_wifi_sta_gw, rt_wifi_ap_dns, rt_wifi_ap_sta_num;

/**
 * W5500 STA模式下的IP地址、子网掩码、网关、DNS服务器配置信息。
 * Configuration information for W5500 STA mode including IP address, subnet mask, gateway, DNS servers.
 */
extern uint32_t rt_w5500_sta_ip, rt_w5500_sta_mask, rt_w5500_sta_gw, rt_w5500_sta_dns1, rt_w5500_sta_dns2, rt_w5500_sta_ipv6_zone;

/**
 * 累计重启次数、最近一次软件主动重启原因、第几次重启、重启前运行时间。
 * 
 * 每一次设备重启都会记录在Flash中，因为有时可能发生异常重启，因此主动重启需要特殊记录。
 * 这些变量用于记录设备重启的相关信息，以便进行故障排查和系统监控。
 *
 * rt_restart_times 表示设备累计重启的总次数。
 * rt_restart_reason 表示最近一次软件主动重启的原因（例如：系统更新、用户请求等）。
 * rt_restart_reason_times 表示最近一次主动重启是第几次重启（帮助区分是正常重启还是异常重启）。
 * rt_restart_reason_seconds 表示设备在最近一次重启之前的运行时间（以秒为单位）。
 *
 * Cumulative restart counts, the reason for the last active software restart, the number of restarts, and the runtime before the last restart.
 *
 * Each device restart is recorded in Flash because sometimes abnormal restarts may occur. Therefore, active restarts need to be specially recorded.
 * These variables are used to record information related to device restarts for troubleshooting and system monitoring purposes.
 *
 * rt_restart_times indicates the total number of restarts the device has undergone.
 * rt_restart_reason indicates the reason for the last active software restart (e.g., system update, user request, etc.).
 * rt_restart_reason_times indicates which restart this was (helping to distinguish between normal and abnormal restarts).
 * rt_restart_reason_seconds indicates the runtime of the device before the last restart (in seconds).
 */

extern uint32_t rt_restart_times, rt_restart_reason, rt_restart_reason_times, rt_restart_reason_seconds;

/**
 * 以下记录当前重启的原因。
 * The following records the reason for the current reset.
 *
 * 每个重启原因都有一个对应的枚举值，用于标识具体的复位原因。
 * Each reset reason has a corresponding enumeration value to identify the specific reset cause.
 *
 * ESP_RST_UNKNOWN (0),    表示无法确定复位原因。
 * ESP_RST_UNKNOWN (0),    Indicates that the reset reason cannot be determined.
 *
 * ESP_RST_POWERON (1),    表示由于电源开启事件导致的复位。
 * ESP_RST_POWERON (1),    Indicates a reset due to a power-on event.
 *
 * ESP_RST_EXT (2),        表示由外部引脚触发的复位（不适用于ESP32）。
 * ESP_RST_EXT (2),        Indicates a reset triggered by an external pin (not applicable for ESP32).
 *
 * ESP_RST_SW (3),         表示通过esp_restart函数进行的软件复位。
 * ESP_RST_SW (3),         Indicates a software reset via the esp_restart function.
 *
 * ESP_RST_PANIC (4),      表示由于异常或崩溃导致的软件复位。
 * ESP_RST_PANIC (4),      Indicates a software reset due to an exception or panic.
 *
 * ESP_RST_INT_WDT (5),    表示由于中断看门狗超时导致的复位（软件或硬件）。
 * ESP_RST_INT_WDT (5),    Indicates a reset due to an interrupt watchdog timeout (software or hardware).
 *
 * ESP_RST_TASK_WDT (6),   表示由于任务看门狗超时导致的复位。
 * ESP_RST_TASK_WDT (6),   Indicates a reset due to a task watchdog timeout.
 *
 * ESP_RST_WDT (7),        表示由于其他看门狗超时导致的复位。
 * ESP_RST_WDT (7),        Indicates a reset due to other watchdog timeouts.
 *
 * ESP_RST_DEEPSLEEP (8),  表示退出深度睡眠模式后的复位。
 * ESP_RST_DEEPSLEEP (8),  Indicates a reset after exiting deep sleep mode.
 *
 * ESP_RST_BROWNOUT (9),   表示由于欠压导致的复位（软件或硬件）。
 * ESP_RST_BROWNOUT (9),   Indicates a brownout reset (software or hardware).
 *
 * ESP_RST_SDIO (10),      表示通过SDIO接口触发的复位。
 * ESP_RST_SDIO (10),      Indicates a reset triggered over the SDIO interface.
 *
 * ESP_RST_USB (11),       表示由USB外设触发的复位。
 * ESP_RST_USB (11),       Indicates a reset triggered by a USB peripheral.
 *
 * ESP_RST_JTAG (12),      表示由JTAG接口触发的复位。
 * ESP_RST_JTAG (12),      Indicates a reset triggered by the JTAG interface.
 *
 * ESP_RST_EFUSE (13),     表示由于efuse错误导致的复位。
 * ESP_RST_EFUSE (13),     Indicates a reset due to an efuse error.
 *
 * ESP_RST_PWR_GLITCH (14),表示由于检测到电源毛刺导致的复位。
 * ESP_RST_PWR_GLITCH (14),Indicates a reset due to a detected power glitch.
 *
 * ESP_RST_CPU_LOCKUP (15),表示由于CPU锁死（双重异常）导致的复位。
 * ESP_RST_CPU_LOCKUP (15),Indicates a reset due to CPU lockup (double exception).
 *
 * rt_reset_reason 记录了设备最近一次复位的具体原因。
 * rt_reset_reason records the specific reason for the last reset of the device.
 */

extern uint32_t rt_reset_reason;

/**
 * 自动重连次数和元数据传输次数。
 * Number of automatic reconnection attempts and metadata transmission counts.
 */
extern uint32_t al_times_reconn, al_meta_times;




/**
 * 设备输入电压值，如果是电池供电，可以用来评估电池余量。
 * The voltage value input by the device can be used to evaluate the remaining battery level in battery-powered applications.
 */
extern int rt_power;

/**
 * 设备上电后，被重启的次数。
 * After the device is powered on, the number of times it has been restarted can be tracked.
 */
extern int rt_power_times;

/**
 * System configuration structure for initialization.
 * 系统初始化配置结构体
 */
typedef struct {
    config_callback callback;           /**< Callback function after config init / 回调函数，在配置初始化后调用 */
    uint32_t config_version;            /**< Configuration version number / 配置版本号 */
    char *server_host;                  /**< Server hostname or IP address / 服务器主机名或IP地址 */
    uint32_t server_port;               /**< Server port number / 服务器端口号 */
    uint8_t upload_timeout_seconds;     /**< Upload timeout in seconds / 上传超时时间（单位：秒） */
    uint16_t keep_alive_idle;           /**< Keep-alive idle time in seconds / 空闲保活时间（单位：秒） */
    uint8_t keep_alive_cnt;             /**< Max number of failed keep-alives before disconnect / 最大失败保活次数 */
    uint8_t keep_alive_interval;        /**< Interval between keep-alive messages in seconds / 每次保活间隔时间（单位：秒） */
    uint32_t no_communicate_seconds;    /**< Time threshold to trigger no-communication alarm (seconds) / 断联报警时间阈值（单位：秒） */
    uint32_t module_max;                /**< Maximum module index used in the system / 系统中使用的最大模块索引 */
    uint32_t sleep_for_x_seconds;            /**< Sleep duration in seconds (0: disable sleep / 单位：秒，0 表示不睡眠) */
    uint32_t sleep_after_running_x_seconds;  /**< Delay before entering sleep (0: enter sleep immediately or disable / 启动后 x 秒进入休眠，0 表示不延迟或禁用该功能) */   
    uint8_t core_log;                   /**< Enable core module logging (0: disable, 1: enable) / 是否启用核心模块日志（0：禁用，1：启用） */
    uint8_t mem_debug;                  /**< Enable memory debug mode (0: disable, 1: enable) / 是否启用内存调试模式（0：禁用，1：启用） */
    uint8_t cloud_log;                  /**< Cloud log level or flag / 云端日志等级或开关标志 */
} system_config_t;

/**
 * 系统内核初始化。
 * 该函数用于初始化系统内核，配置必要的参数以启动系统。
 * 函数参数包括用户配置的回调函数、配置版本号、云端主机地址、云端主机端口以及日志相关选项。
 * 
 * This function initializes the system kernel and configures necessary parameters to start the system.
 * The parameters include a user-defined callback function, configuration version, cloud host address, 
 * cloud host port, and logging options.
 */
void system_init(const system_config_t *config);

/**
 * 系统内核启动。
 * 该函数用于启动系统内核，完成初始化后的启动流程。
 * 调用此函数后，系统将进入正常运行状态并开始执行核心任务。
 * 
 * This function starts the system kernel and completes the startup process after initialization.
 * Once called, the system enters normal operation and begins executing core tasks.
 */
void system_start(void);

/**
 * 计算MD5哈希值，但不是普通MD5，内部会增加额外处理以保证通讯的不可修改性。
 * 该函数用于计算输入数据的MD5哈希值，并在生成过程中加入额外的安全处理，以增强数据完整性和防篡改能力。
 * 
 * content 是需要计算的输入数据，通常为二进制格式或字符串。
 * content_len 是输入数据的长度，单位为字节，用于确定计算范围。
 * md5_value 是存储输出的哈希值的缓冲区，结果以二进制格式存储，而非常见的十六进制（hex）格式。
 *           一般情况下，MD5哈希值为16个字节，如果转换为十六进制格式，则是32个字符。
 * md5_len 指定返回的哈希值的字节数。例如，输入16表示返回完整的16字节哈希值；输入8则只返回后8个字节。
 *         这种灵活性可以满足不同的应用场景需求，例如减少传输数据量或提高效率。
 * 
 * This function calculates the MD5 hash value, but it is not a standard MD5 calculation. Additional processing 
 * is performed internally to ensure the integrity and non-modifiability of the communication.
 * 
 * content: The input data to be hashed, typically in binary format or as a string.
 * content_len: The length of the input data in bytes, used to determine the calculation range.
 * md5_value: The buffer to store the resulting hash value, which is stored in binary format rather than the 
 *            common hexadecimal (hex) format. A standard MD5 hash is 16 bytes long, which would be represented 
 *            as 32 characters in hex format.
 * md5_len: Specifies how many bytes of the hash value should be returned. For example, inputting 16 returns the 
 *          full 16-byte hash, while inputting 8 returns only the last 8 bytes. This flexibility can meet different 
 *          application requirements, such as reducing data transmission size or improving efficiency.
 */
uint8_t *md5_get(uint8_t *content, uint16_t content_len, uint8_t *md5_value, uint8_t md5_len);

/**
 * 反向搜索目标字节。
 * 该函数用于在给定数据中从后向前搜索目标字节，并返回其位置。
 * 参数包括数据指针、数据长度、目标字节以及起始索引。
 * 返回值为目标字节的位置指针，若未找到则返回NULL。
 * 
 * data 数据指针，指向需要搜索的数据块。
 * data_len 数据长度，单位为字节。
 * target 目标字节，表示需要搜索的具体值。
 * index 起始索引，表示从哪个位置开始反向搜索。
 * 
 * This function searches for a target byte in the given data from the end to the beginning and returns its position.
 * Parameters include the data pointer, data length, target byte, and starting index.
 * The return value is a pointer to the position of the target byte, or NULL if not found.
 * 
 * data: Pointer to the data block to be searched.
 * data_len: Length of the data in bytes.
 * target: The target byte to search for.
 * index: Starting index for the reverse search.
 */
uint8_t *byte_search_reverse(uint8_t *data, uint32_t data_len, uint8_t target, uint32_t index);

/**
 * 查找子字符串。
 * 该函数用于在给定数据中查找指定的子字符串，并返回其位置。
 * 参数包括起始指针、数据长度和目标字符串。
 * 返回值为目标字符串的位置指针，若未找到则返回NULL。
 * 
 * start 起始指针，指向需要搜索的数据块的起始位置。
 * data_len 数据长度，单位为字节。
 * str 目标字符串，表示需要查找的具体内容。
 * 
 * This function searches for a specified substring in the given data and returns its position.
 * Parameters include the starting pointer, data length, and target string.
 * The return value is a pointer to the position of the target substring, or NULL if not found.
 * 
 * start: Pointer to the starting position of the data block to be searched.
 * data_len: Length of the data in bytes.
 * str: The target substring to search for.
 */
uint8_t *util_memmem(uint8_t *data, uint32_t data_len, char *str);
/**
 * 把data所指向的二进制数据转换为十六进制（hex）格式。
 * 该函数将输入的二进制数据逐字节转换为十六进制字符串形式，每个字节以%02x格式表示。
 * 转换后的结果直接覆盖原始二进制数据，因此无需额外的存储空间来保存返回值。
 * 由于每个字节会被转换为两个字符（例如0xFF变为"FF"），最终数据的长度会变为原来的两倍。
 * 因此，在调用该函数之前，必须确保data的空间足够容纳转换后两倍长度的数据，否则可能导致内存越界。
 * 
 * data 是需要转换的二进制数据指针，其内容在转换后会被覆盖。
 * data_len 是需要转换的二进制数据的长度，单位为字节，用于确定转换范围。
 * 
 * This function converts the binary data pointed to by data into hexadecimal (hex) format.
 * Each byte of the input binary data is converted into a two-character hexadecimal string in %02x format.
 * The result of the conversion directly overwrites the original binary data, so no additional storage space 
 * is required for the return value. Since each byte is expanded into two characters (e.g., 0xFF becomes "FF"), 
 * the final length of the data will be twice the original length. Therefore, before calling this function, it 
 * is essential to ensure that the data buffer has enough space to accommodate the doubled length of the data, 
 * otherwise memory overflow may occur.
 * 
 * data: Pointer to the binary data to be converted. The original content will be overwritten after conversion.
 * data_len: Length of the binary data in bytes, used to determine the range of conversion.
 */
uint8_t *bin_hex(uint8_t *data, uint32_t data_len);
/**
 * 把str字符串转换为uint32_t格式。
 * 该函数用于将输入的字符串按照其内容解析为一个32位无符号整数（uint32_t）。
 * 转换的过程会根据字符串的实际内容进行解析，通常假设字符串表示的是一个有效的数字。
 * 如果字符串的内容无法正确解析为数字，则结果可能是未定义的，因此需要确保输入字符串的合法性。
 * 
 * str 是输入的字符串指针，指向需要转换的字符串数据。
 * str_len 是输入字符串的长度，用于确定需要解析的字符范围。
 * 
 * This function converts the input string str into a 32-bit unsigned integer (uint32_t).
 * The conversion process parses the content of the string, assuming it represents a valid number.
 * If the content of the string cannot be correctly parsed as a number, the result may be undefined. 
 * Therefore, it is important to ensure that the input string is valid and properly formatted.
 * 
 * str: Pointer to the input string that needs to be converted.
 * str_len: Length of the input string, used to determine the range of characters to parse.
 */
uint32_t str_to_num(uint8_t *str, uint32_t str_len);

/**
 * 获取系统运行时间（以秒为单位）。
 * 该函数用于返回系统自启动以来的运行时间，通常基于内部计时器或系统时钟计算。
 * 返回值是一个32位无符号整数，表示从系统启动到当前时刻经过的秒数。
 * 
 * This function retrieves the system's runtime in seconds since it was started. 
 * The value is typically calculated based on an internal timer or system clock.
 * The return value is a 32-bit unsigned integer representing the number of seconds elapsed since system startup.
 */
uint32_t util_get_run_seconds(void);

/**
 * 延迟指定的毫秒数。
 * 该函数用于实现精确的延迟操作，输入参数为需要延迟的毫秒数。
 * 在延迟期间，系统可能会进入低功耗模式或执行其他后台任务，具体行为取决于硬件和操作系统实现。
 * 
 * This function implements a precise delay operation, with the input parameter specifying the number of milliseconds to delay.
 * During the delay, the system may enter a low-power mode or perform other background tasks, depending on the hardware and OS implementation.
 */
void util_delay_ms(uint32_t timeinms);

/**
 * 系统重启。
 * 该函数用于触发系统重启操作，输入参数flag可以携带额外的标志信息，用于指示重启的原因或模式。
 * 该flag会首先被存储到Flash中，在系统重启后会自动读取，并发送到云端。通过这种方式，云端能够掌握设备重启的具体原因。
 * 
 * 以下列出了目前已经使用的flag值及其含义，新增flag时需要按照顺序添加：
 * 0: 表示未知原因（unknown）。
 * 1-2: 表示4G模块错误，分别对应不同的错误类型（4g modem error 1 和 4g modem error 2）。
 * 3: 表示配置更改导致的重启（config change）。
 * 4: 表示动态内存分配失败（user_malloc）。
 * 5-8: 表示SPI通信错误（spi error），具体错误类型由数值区分。
 * 9: 表示长时间无通信导致的重启（no commu）。
 * 10: 表示进入安全配置模式（safe config）。
 * 11-18: 表示OTA升级相关错误，包括开始、写入、校验、设置启动分区等阶段的问题。
 *        具体包括：esp_ota_begin（11）、esp_ota_write（12）、ota md5校验错误（13）、
 *        ota镜像验证（14）、设置启动分区（15）、OTA成功完成（16）、回滚（17）、esp_ota_end（18）。
 * 19: 表示网络设置恢复后触发的重启（network restore）。
 * 20: 表示BLE模块触发的重启（ble reboot）。
 * 
 * This function triggers a system reboot. The input parameter flag carries additional information indicating the reason or mode of the reboot.
 * The flag is first stored in Flash memory and automatically retrieved after the system reboots. It is then sent to the cloud, allowing the cloud 
 * to track the specific cause of the device reboot.
 * 
 * The following lists the currently used flag values and their meanings. When adding new flags, they should be appended in sequence:
 * 0: Indicates an unknown reason (unknown).
 * 1-2: Indicate errors related to the 4G module, corresponding to different error types (4g modem error 1 and 4g modem error 2).
 * 3: Indicates a reboot caused by configuration changes (config change).
 * 4: Indicates a failure in dynamic memory allocation (user_malloc).
 * 5-8: Indicate SPI communication errors (spi error), with specific error types distinguished by the value.
 * 9: Indicates a reboot triggered by prolonged lack of communication (no commu).
 * 10: Indicates entering safe configuration mode (safe config).
 * 11-18: Indicate errors related to OTA upgrades, including issues during the start, write, verification, and boot partition setup phases.
 *        Specifically: esp_ota_begin (11), esp_ota_write (12), ota md5 verification error (13), 
 *        ota image validation (14), setting the boot partition (15), OTA successfully completed (16), rollback (17), esp_ota_end (18).
 * 19: Indicates a reboot triggered after network config restoration (network restore).
 * 20: Indicates a reboot triggered by the BLE module (ble reboot).
 */
void util_reboot(uint32_t flag);

/**
 * 和util_reboot类似，该函数也会将重启标志写入Flash，但不会真正执行系统重启操作。
 * 该函数的主要作用是记录重启标志到Flash中，以便在后续的系统运行中可以读取并处理这些标志。
 * 输入参数flag的含义与util_reboot中的相同，用于指示特定的原因或模式。
 * 这种机制可以用于延迟重启的场景，例如在某些情况下需要先完成关键任务后再触发重启。
 * 
 * This function is similar to util_reboot in that it also writes the reboot flag to Flash, but it does not actually perform a system reboot.
 * Its primary purpose is to record the reboot flag in Flash so that it can be read and processed during subsequent system operation.
 * The meaning of the input parameter flag is the same as in util_reboot, indicating specific reasons or modes.
 * This mechanism can be used in scenarios where a reboot needs to be delayed, such as when certain critical tasks must be completed before triggering a reboot.
 */
void util_non_reboot(uint32_t flag);

/**
 * 释放动态分配的内存。
 * 该函数是对标准C库free函数的封装，用于释放由user_malloc分配的内存块。
 * 如果传入的指针为NULL，则函数不会执行任何操作。
 * 使用此函数可以确保内存管理的一致性，避免直接调用free带来的潜在问题。
 * 
 * This function is a wrapper around the standard C library free function, used to release memory blocks allocated by user_malloc.
 * If the input pointer is NULL, the function will do nothing.
 * Using this function ensures consistent memory management and avoids potential issues from directly calling free.
 */
void user_free(void *p);

/**
 * 动态分配内存。
 * 该函数是对标准C库malloc函数的封装，用于分配指定大小的内存块。
 * 返回值是一个指向分配内存的指针。如果分配失败，设备会直接重启，以避免因内存不足导致的系统异常。
 * 分配的内存会被初始化为0，确保使用时不会出现未定义的行为。
 * 使用此函数可以确保内存分配的一致性，并便于在特定场景下进行扩展或调试。
 * 
 * This function is a wrapper around the standard C library malloc function, used to allocate a memory block of the specified size.
 * The return value is a pointer to the allocated memory. If the allocation fails, the device will directly reboot to prevent system anomalies 
 * caused by insufficient memory. The allocated memory is initialized to 0 to ensure no undefined behavior occurs during use.
 * Using this function ensures consistent memory allocation and facilitates extension or debugging in specific scenarios.
 */
void *user_malloc_exe(const char *file,const uint32_t line, const char * func, uint32_t size);
#define user_malloc(size) user_malloc_exe(__FILE__, __LINE__, __func__,size)

/**
 * 配合system_init打开mem_debug开关时使用，用于调试内存分配和释放情况。
 * 如果在系统初始化时开启了内存调试（mem_debug）功能，系统会自动记录每次内存申请和释放的操作。
 * 调用该函数可以打印出那些申请后未被释放的内存块信息，帮助开发者定位潜在的内存泄漏问题。
 * 参数min_count用于过滤输出结果，只有当某块内存的申请次数减去释放次数大于min_count时才会被打印。
 * 设置min_count可以有效减少输出内容的数量，避免因打印过多信息而干扰调试过程。
 * 
 * This function is used in conjunction with the mem_debug switch enabled in system_init to debug memory allocation and deallocation.
 * When the memory debugging (mem_debug) feature is enabled during system initialization, the system automatically records each memory 
 * allocation and deallocation operation. Calling this function prints information about memory blocks that were allocated but not freed, 
 * helping developers identify potential memory leaks. The parameter min_count filters the output: only memory blocks where the number of 
 * allocations minus the number of deallocations exceeds min_count will be printed. Setting min_count effectively reduces the amount of 
 * output, avoiding excessive information that could interfere with the debugging process.
 */
void user_malloc_print(uint32_t min_count);

#ifdef __cplusplus
}
#endif
