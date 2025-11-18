#pragma once



#include "freertos/FreeRTOS.h"
#include "device_config.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * 设备与云端维持TCP/IP连接，通过报文进行数据交互，内核系统会创建两个线程任务，一个用于发送，一个用于接收。
 * The device maintains a TCP/IP connection with the cloud for data interaction via messages. The kernel system creates two thread tasks, one for sending and one for receiving.
 */
 
/**
 * 每次发送一个报文，都要注册一个handler用于接收报文发送后，云端返回内容的处理。内核系统会自动回收这些handler,
 * 如果有些数据发送后，对应的handler一直未接收到云端的返回乃至超时，内核系统会自动清理这些handler。
 * The handler must be registered each time a message is sent to handle the response from the cloud. The kernel system automatically recycles these handlers.
 * If a handler does not receive a response from the cloud and times out, the kernel system will automatically clean up these handlers.
 *
 * 以下定义handler是一次性使用还是重复使用，设备主动发送都是创建一次使用handler，但系统启动时会自动创建一个多次使用的base handler用于接收云端随时可能主动下发的报文。
 * The following defines whether the handler is for one-time use or reusable. Handlers created for device-initiated sends are for one-time use, but at system startup, a reusable base handler is automatically created to receive messages that the cloud may initiate at any time.
 */
typedef enum {
    /**
     * 表示该handler可以被多次使用，适用于系统启动时创建的基础handler来接收云端主动下发的数据。
     * indicates that this handler can be reused multiple times, suitable for the base handler created at system startup to receive data initiated by the cloud.
     */
    SOCKET_HANDLER_REUSE = 0,

    /**
     * 表示该handler仅能使用一次，适用于设备主动发送报文后创建的handler。
     * indicates that this handler can only be used once, suitable for handlers created after the device initiates sending a message.
     */
    SOCKET_HANDLER_ONE_TIME = 1
} socket_handler_reuse_status;

/**
 * 系统的主线程会周期轮询各个模块，操作模块来读取传感器数据，然后上报。云端也可能随时下发报文来操作模块，比如下发指令给AHT20模块读取温湿度数据。
 * 为了让AHT20在线程安全的环境下运行，主线程和处理云端下发报文的线程要通过以下函数锁定和释放锁。
 * The main thread of the system periodically polls each module to read sensor data and then report it. The cloud may also send messages at any time to operate the modules, such as sending commands to the AHT20 module to read temperature and humidity data.
 * To ensure that the AHT20 operates in a thread-safe environment, the main thread and the thread handling cloud messages must use the following functions to lock and release locks.

 * 获取锁，
 * 如module_lock("module",MODULE_AHT20,portMAX_DELAY)，表示一直等待，直到获取到MODULE_AHT20锁
 * 如module_lock("module",MODULE_AHT20,0)，表示如果不能立刻获取到锁，就直接返回
 * 如module_lock("module",MODULE_AHT20,pdMS_TO_TICKS(200))，表示最多等待200毫秒，之后就返回
 * comments用户调试目的，可以随便输入或者NULL
 * 返回1表示获取锁成功，0表示失败
 * To acquire a lock,
 * e.g., module_lock("module", MODULE_AHT20,portMAX_DELAY) means wait indefinitely until the lock is acquired for MODULE_AHT20.
 * e.g., module_lock("module", MODULE_AHT20,0) means return immediately if the lock cannot be acquired right away.
 * e.g., module_lock("module", MODULE_AHT20,pdMS_TO_TICKS(200)) means wait up to 200 milliseconds before returning.
 * comments are for debugging purposes and can be any string or NULL.
 * Returns 1 if the lock is successfully acquired, 0 otherwise.
 */
uint8_t module_lock(char *comments,uint32_t module,TickType_t x);

/**
 * 释放锁
 * comments用户调试目的，可以随便输入或者NULL
 * module 模块的ID
 * To release a lock.
 * comments are for debugging purposes and can be any string or NULL.
 * module ID of module
 */
void module_unlock(char *comments,uint32_t module);




/**
 * 方便把要发送的data和data_len打包成data_send_container。
 * This function conveniently packages the data and data_len to be sent into a data_send_container.
 *
 * 它接受原始数据及其长度，并将其封装到一个结构体中，便于后续处理。
 * It accepts raw data and its length, packaging them into a structure for easier subsequent processing.
 *
 * need_free指定在释放data_send_container时是否需要释放内部数据。
 * need_free specifies whether the internal data should be freed when the data_send_container is freed.
 *
 * command表示命令请求类型。
 * command indicates the type of command request.
 *
 * data是要发送的数据。
 * data is the data to be sent.
 *
 * data_len是数据的长度。
 * data_len is the length of the data.
 */

data_send_container* data_container_create(uint8_t need_free, command_request command, uint8_t *data, uint32_t data_len,socket_recv_callback callback);

/**
 * 将数据添加到现有的data_send_container中。
 * Add data to an existing data_send_container.
 *
 * 此函数允许用户向已经存在的data_send_container结构体中追加新的数据。
 * This function allows users to append new data to an existing data_send_container structure.
 *
 * container是要添加数据的目标容器。
 * container is the target container to which data will be added.
 *
 * need_free指定在释放data_send_container时是否需要释放新添加的数据。
 * need_free specifies whether the newly added data should be freed when the data_send_container is freed.
 *
 * data是需要添加的数据。
 * data is the data to be added.
 *
 * data_len是数据的长度。
 * data_len is the length of the data.
 */
void data_container_add_data(data_send_container *container, uint8_t need_free, uint8_t *data, uint32_t data_len);

/**
 * 向云端发送数据，发送成功后立刻返回。返回结果会被接收任务线程自动处理。
 * 因为只是确认云端是否收到或者配置或者OTA升级，因此大部分情况下内核自动处理即可。
 * 内核收到返回报文后也会在处理前回调comm_util.c的response_process函数，
 * 因此如果用户希望对返回结果进行处理，可以修改这个函数。
 * 发送的报文和返回的报文都有相同的pack_seq，可以用来关联。
 *
 * Send data to the cloud and immediately return upon successful sending. 
 * The return result will be automatically processed by the receiving task thread.
 * Because it is only to confirm whether the cloud has received the data, 
 * configuration, or OTA updates, in most cases, automatic handling by the kernel is sufficient.
 * The core system will also callback the `response_process` function in `comm_util.c` 
 * before processing the return message. Therefore, if users wish to handle the return result, 
 * they can modify this function. Both the sent and returned messages have the same `pack_seq`, 
 * which can be used to associate them.

 * handler_name给本次发送取个名字，方便调试；
 * data_container要发送的信息；
 * params，params_len传给handler的参数和参数长；
 * flag类型为command_flag，参考这个枚举的定义；
 * socket_handler_reuse_status参考枚举定义；
 * pack_seq_in如果输入0，核心系统用自增长ID，普通发送直接输入0即可，
 * 如果非0数值，指定发送的pack_seq，一般用于响应云端的报文用；
 * pack_seq_out核心系统发送报文用的ID的值，如果用户希望对返回内容在response_process里进行检查，可以比对这个值。
 * 返回0表示成功，小于0表示失败。
 * 
 * handler_name gives a name to this send operation for easier debugging;
 * data_container contains the information to be sent;
 * params and params_len are the parameters and their lengths passed to the handler;
 * flag is of type command_flag, refer to the definition of this enum;
 * socket_handler_reuse_status refers to the definition of this enum;
 * If pack_seq_in is 0, the core system uses an auto-incrementing ID; for normal sends, 
 * just input 0. If a non-zero value is provided, it specifies the `pack_seq` to be used, 
 * typically for responding to cloud messages; pack_seq_out is the ID value used by the core 
 * system when sending the message. If users wish to check the return content in the 
 * `response_process` function, they can compare this value.
 * Return 0 indicates success, a value less than 0 indicates failure.
 */
int tcp_send_command(char *handler_name, data_send_container *data_container, 
                     void *params, uint32_t params_len, uint32_t flag, 
                     socket_handler_reuse_status one_time, uint32_t pack_seq_in, 
                     uint32_t *pack_seq_out);
                     
/**
 * 发送本地配置信息到云端，内部也是调用tcp_send_command，只是一个方便发送配置的函数。
 * Send local configuration information to the cloud. This function internally calls `tcp_send_command` 
 * and is simply a convenient function for sending configurations.
 *
 * handler_name handler名称；
 * handler_name is the name of the handler;
 *
 * params传给handler的参数值；
 * params is the value of the parameter passed to the handler;
 *
 * params_len传给handler的参数长度。
 * params_len is the length of the parameter passed to the handler.
 * 返回0表示成功，小于0表示失败。
 * Return 0 indicates success, a value less than 0 indicates failure.
 */
int tcp_send_config(char *handler_name, uint8_t *params, uint8_t params_len);               

#ifdef __cplusplus
}
#endif
