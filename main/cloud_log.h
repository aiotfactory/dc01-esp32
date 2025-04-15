#pragma once


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * 嵌入式设备日志查看一般都要通过连接导线到电脑串口查看，
 * 本设备提供云端查看日志方式，原理就是设备把日志通过网络模块发送到云端平台。
 * 因此用户代码可以调用如下函数来输出日志，不但要调用如下函数，
 * 还要通过void system_init(config_callback callback, uint8_t enable_log, 
 * uint8_t enable_mem_debug, uint8_t enable_cloud_log)配置开启云端日志。
 * 开启后，日志发送到云端的同时也会通过串口输出。
 * 因为发送云端需要网络流量，因此在使用4G SIM卡时要小心开启，避免流量被耗光。
 *
 * Viewing logs on embedded devices typically requires connecting a cable to the computer's serial port.
 * This device provides cloud-based log viewing. The principle is that the device sends logs to the cloud platform via a network module.
 * Therefore, user code can call the following functions to output logs. Not only should these functions be called,
 * but cloud logging must also be enabled through void system_init(config_callback callback, uint8_t enable_log, 
 * uint8_t enable_mem_debug, uint8_t enable_cloud_log). Once enabled, logs will be sent to the cloud and also output through the serial port.
 * Since sending logs to the cloud consumes network traffic, caution should be exercised when enabling this feature with a 4G SIM card to avoid exhausting your data allowance.
 */
 
/**
 * 有时候即便开启了云端日志配置，也希望有些日志不需要被发送到云端。
 * 可以调用此函数，此函数只通过串口打印日志，不会发送到云端，无论云端日志是否开启。
 *
 * 参数说明：
 * - prefix: 打印一些前缀信息。
 * - suffix: 打印一些后缀信息。
 * - format: 类似printf的format定义，用于格式化输出。
 * - data: 要打印的内容。
 * - datalen: 要打印的数据长度。
 *
 * 此函数可以灵活地在代码中添加自定义的日志输出，仅限于串口输出，不涉及网络传输。
 *
 * Even if cloud logging is enabled, sometimes you may want certain logs not to be sent to the cloud.
 * You can call this function, which only prints logs through the serial port and does not send them to the cloud, regardless of whether cloud logging is enabled.
 *
 * Parameter descriptions:
 * - prefix: Print some prefix information.
 * - suffix: Print some suffix information.
 * - format: Similar to printf's format definition, used for formatted output.
 * - data: The content to be printed.
 * - datalen: The length of the data to be printed.
 *
 * This function allows flexible addition of custom log outputs in the code, limited to serial port output, without involving network transmission.
 */
void util_printf_format(char *prefix, char *suffix, char *format, uint8_t *data, uint32_t datalen);

/**
 * 如果开启了云端日志，该函数会把日志发送到云端；
 * 如果没有配置开启云端日志，则该函数的行为与util_printf_format相同，仅通过串口打印日志。
 * 另外，此函数最终打印的内容长度最大为1024字节，如果超过此长度，会被截取。
 *
 * 参数说明：
 * - prefix: 打印一些前缀信息。
 * - suffix: 打印一些后缀信息。
 * - format: 类似printf的format定义，用于格式化输出。
 * - data: 要打印的内容。
 * - datalen: 要打印的数据长度。
 *
 * 注意：此函数在云端日志开启时会将日志发送到云端，并且也会通过串口输出日志。最终打印的内容长度最大为1024字节，超出部分将被截取。
 *
 * If cloud logging is enabled, this function sends logs to the cloud.
 * If cloud logging is not configured to be enabled, this function behaves like util_printf_format, printing logs only through the serial port.
 * Additionally, the final printed content length is capped at 1024 bytes; if it exceeds this limit, it will be truncated.
 *
 * Parameter descriptions:
 * - prefix: Print some prefix information.
 * - suffix: Print some suffix information.
 * - format: Similar to printf's format definition, used for formatted output.
 * - data: The content to be printed.
 * - datalen: The length of the data to be printed.
 *
 * Note: This function sends logs to the cloud when cloud logging is enabled and also prints logs through the serial port. The final printed content length is capped at 1024 bytes, with any excess being truncated.
 */
void cloud_printf_format(char *prefix, char *suffix, char *format,uint8_t *data,uint32_t datalen);

/**
 * 类似于printf，只是当云端日志配置开启后，打印的同时也会发送到云端。
 * 此函数内部会动态申请内存以容纳要发送的内容，因此不宜一次打印过长的日志，以免造成内存压力或耗尽内存资源。
 *
 * 参数说明：
 * - format: 格式化字符串，类似于printf的格式定义。
 * - ...: 可变参数列表，根据format中的格式符提供相应的参数。
 *
 * 注意：此函数在云端日志开启时会将日志发送到云端，并且也会通过串口输出日志。由于内部使用动态内存分配，建议避免一次性打印过长的日志。
 *
 * Similar to printf, but when cloud logging is enabled, it also sends the logs to the cloud.
 * This function dynamically allocates memory to accommodate the content to be sent, so it is not advisable to print overly long logs at once to avoid memory pressure or exhausting memory resources.
 *
 * Parameter descriptions:
 * - format: Formatting string, similar to printf's format definition.
 * - ...: Variadic arguments list, providing parameters according to the format specifiers in the format string.
 *
 * Note: This function sends logs to the cloud when cloud logging is enabled and also prints logs through the serial port. Since it uses dynamic memory allocation internally, it is recommended to avoid printing excessively long logs at one time.
 */
void cloud_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif
