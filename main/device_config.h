#pragma once


#ifdef __cplusplus
extern "C"
{
#endif



/**
 * 定义PCB上使用的SPI主机控制器
 * Define the SPI host used on the PCB.
 */
#define SPI_HOST SPI2_HOST // 使用SPI2作为主机控制器 / Here using SPI2 as the host controller

/**
 * 定义SPI通信的相关引脚，对应ESP芯片的GPIO编号
 * Define SPI communication related pins with corresponding GPIO numbers of the ESP chip.
 */
#define SPI_MISO 39       /**< SPI MISO (Master In Slave Out) 引脚 / SPI MISO pin. */
#define SPI_MOSI 38       /**< SPI MOSI (Master Out Slave In) 引脚 / SPI MOSI pin. */
#define SPI_SCK 40        /**< SPI SCK (Serial Clock) 引脚 / SPI SCK pin. */
#define SPI_DUMMY_CS -1    /**< DUMMY CS (Chip Select) 引脚，未实际连接设备，仅作示例 / DUMMY CS pin, not actually connected, for demonstration purposes. */

/**
 * 定义以太网接口的中断引脚
 * Define the Ethernet interface interrupt pin with the corresponding GPIO number of the ESP chip.
 */
#define ETH_INT 47        /**< 以太网接口的中断引脚 / Ethernet interface interrupt pin. */

/**
 * 定义LORA模块的相关引脚，对应ESP芯片的GPIO编号
 * Define LORA module related pins with corresponding GPIO numbers of the ESP chip.
 */
#define LORA_BUSY 48      /**< LORA模块的BUSY引脚，指示模块是否忙碌 / LORA module BUSY pin, indicates if the module is busy. */
#define LORA_DIO1 45      /**< LORA模块的DIO1引脚，用于接收数据包到达等信号 / LORA module DIO1 pin, used to receive signals like packet arrival. */


/**
 * 嵌入式设备上会有很多配置信息，比如上传数据的云端主机IP，端口，是否开启某些传感器等。
 * 用户可以在device_config.c文件中函数void define_user_config(void)内定义设备的配置。
 * 设备启动时会回调该函数。
 *
 * 整体架构思路是，设备会划分很多模块，定义在枚举类型config_module中，比如BLE模块,4G模块和LORA模块等，
 * 每个模块的配置单独配置。每个配置项会有一个唯一枚举编号，定义在枚举类型config_name中，
 * 每一个配置项还会有名称，默认值，类型，范围，以及该配置项如果云端通知改变时该采取什么措施，
 * 比如重启，或者网络重新连接等。
 *
 * 所有配置内容设备都会按一定格式发送到云端，增加配置项时，并不需要在云端也同时定义，
 * 因为设备把配置项的属性信息都发送到云端，云端只要解析保存即可。
 * 这样的架构非常灵活，用户可以随意增加或者减少配置项。
 *
 * 有一些系统内核已经配置的内容如上报主机的IP和端口用户无法删除。
 *
 * 如果使用者在云端修改了配置项的内容，云端首先会根据配置项定义的范围检查是否符合要求，如果符合要求会先保存在云端，
 * 之后云端会通过网络下发到设备，设备接收后和本地配置内容对比，如果相同就忽略，如果不同就修改本地配置，
 * 如果配置项设置为内容变化后重启，设备就自我重启。
 *
 * Embedded devices have many configuration items, such as the IP and port of the cloud host for uploading data,
 * whether to enable certain sensors, etc. Users can define device configurations within the function void define_user_config(void)
 * in the device_config.c file. This function is called back when the device starts up.
 *
 * The overall architecture concept is that the device will be divided into many modules defined in the enum config_module,
 * such as BLE module, 4G module, LORA module, etc., with each module configured separately. Each configuration item has a unique
 * enumeration number defined in the enum config_name. Each configuration item also includes a name, default value, type, range,
 * and actions to take if the configuration changes on the cloud, such as rebooting or reconnecting the network.
 *
 * All configuration contents are sent to the cloud in a specific format. When adding new configuration items, there's no need
 * to define them simultaneously on the cloud because the device sends all attribute information of the configuration items to the cloud,
 * which only needs to parse and save them. This architecture is very flexible, allowing users to freely add or remove configuration items.
 *
 * Some system kernel-configured contents, such as the IP and port of the reporting host, cannot be deleted by users.
 *
 * If a user modifies the configuration item content on the cloud, the cloud first checks whether it meets the requirements defined by the configuration item's range.
 * If it meets the requirements, it will be saved on the cloud. Subsequently, the cloud will send the updated configuration down to the device via the network.
 * Upon receiving the update, the device compares it with the local configuration. If they are the same, it will be ignored; if different, the local configuration will be modified.
 * If the configuration item is set to reboot upon content change, the device will self-reboot.
 */
 

typedef enum {
    /**
     * 表示为普通数字，比如端口号，上报间隔秒数这些适合使用。
     * Indicates a normal number, suitable for use with values like port numbers and reporting interval seconds.
     */
    CONFIG_NUMBER_NORMAL = 0,

    /**
     * 数字枚举类型，比如xx传感器是否开启，可以用0,1表示关闭和开启。
     * Enumerated number type, such as whether an xx sensor is enabled, can be represented by 0 (off) and 1 (on).
     */
    CONFIG_NUMBER_ENUM = 1,

    /**
     * 普通字符串类型，比如WIFI的SSID名称适用。
     * Normal string type, suitable for values like the SSID name of a WIFI network.
     */
    CONFIG_STRING_NORMAL = 51,

    /**
     * IP V4字符串。
     * IP V4 string.
     */
    CONFIG_STRING_IP_V4 = 52,

    /**
     * IP V6字符串。
     * IP V6 string.
     */
    CONFIG_STRING_IP_V6 = 53,

    /**
     * IP或者域名，比如云端服务器地址适用。
     * IP or domain name, suitable for values like the address of a cloud server.
     */
    CONFIG_STRING_IP_URL = 54
} config_type;

 
/**
 * 以下是用到的一些配置flag,
 * The following are some configuration flags used:
 */

typedef enum {
    /**
     * 配置项设置为该值时，如果云端通知设备该配置项改变，设备保存配置项后会自我重启。
     * When the configuration item is set to this value, if the cloud notifies the device of changes to the configuration item, the device will reboot after saving the configuration.
     */
    CONFIG_FLAG_REBOOT = 1 << 0,

    /**
     * 配置项设置为该值时，如果云端通知设备该配置项改变，设备保存配置项后会断开和云端连接重新连接。
     * When the configuration item is set to this value, if the cloud notifies the device of changes to the configuration item, the device will disconnect and reconnect to the cloud after saving the configuration.
     */
    CONFIG_FLAG_RECONN = 1 << 1,

    /**
     * 内部使用，表明本次云端通知是否有配置项发生了改变，一般用户不需要使用。
     * For internal use, indicates whether any configuration items have changed in this cloud notification. Generally not used by users.
     */
    CONFIG_FLAG_CHANGED = 1 << 2,

    /**
     * 内部使用，表明本次云端通知的配置项是否不符合配置项的范围要求。
     * For internal use, indicates whether the configuration items in this cloud notification do not meet the range requirements.
     */
    CONFIG_FLAG_ERROR = 1 << 3,

    /**
     * 内部使用，表明本次云端通知的配置项是否和本地配置有任何变化。
     * For internal use, indicates whether the configuration items in this cloud notification are the same as the local configuration.
     */
    CONFIG_FLAG_SAME = 1 << 4
} config_flag;


/**
 * 定义一个具体配置项的结构，比如云端服务器地址是一个配置项，云端服务器端口是另外一个配置项。
 * Define a structure for a specific configuration item, such as the cloud server address being one configuration item and the cloud server port being another.
 */

typedef struct {
    /**
     * 配置项名称。Configuration item name.
     */
    char *name;

    /**
     * 配置项类型。Configuration item type.
     */
    uint8_t type;

    /**
     * config_flag 配置项标志。Configuration item flag.
     */
    uint8_t flag;

    /**
     * 存储配置项的范围，用户不需要处理。Stores the range of the configuration item, no need for user handling.
     */
    uint8_t *range;

    /**
     * 内部存状态用，用户不需要处理。Used internally to store status, no need for user handling.
     */
    uint8_t status;

    /**
     * 配置项所属的模块。Module to which the configuration item belongs.
     */
    uint32_t module;

    /**
     * 联合体用于存储配置项的值，根据类型不同可以是数值或字符串。
     * Union used to store the value of the configuration item, can be either a number or a string depending on the type.
     */
    union {
        /**
         * 如果是数值。If it is a numeric value.
         */
        uint32_t content;

        /**
         * 如果是字符串。If it is a string.
         */
        void *pointer;
    } value;

    /**
     * 数据长度。Data length.
     */
    uint32_t value_len;
} config_element;

/**
 * 以下为每个配置项的编号枚举，具体配置内容用户可以增加，默认的如下，其定义在云端相关文档中说明，这里不再重复。
 * The following are enumeration numbers for each configuration item. Users can add specific configuration contents. Defaults are listed below, which are explained in the cloud-related documentation and will not be repeated here.
 */
 
typedef enum {
	//核心系统定义的枚举，不能修改。The system-defined enumeration values, which cannot be modified.
	CONFIG_SYSTEM_VERSION,
    CONFIG_SYSTEM_SERVER_HOST,
    CONFIG_SYSTEM_SERVER_PORT,
    CONFIG_SYSTEM_CLOUD_LOG_ONOFF,
    CONFIG_SYSTEM_UPLOAD_TIMEOUT_SECONDS,
    CONFIG_SYSTEM_KEEP_ALIVE_IDLE,
    CONFIG_SYSTEM_KEEP_ALIVE_CNT,
    CONFIG_SYSTEM_KEEP_ALIVE_INTERVAL,
    CONFIG_SYSTEM_NO_COMMUNICATE_SECONDS,
    CONFIG_SYSTEM_SLEEP_AFTER_RUNNING_X_SECONDS,
    CONFIG_SYSTEM_SLEEP_FOR_X_SECONDS,
    //以下是用户定义的枚举，可以增删或修改  //Below are the user-defined enumeration values, which can be added to but cannot have their numerical values explicitly set.
    CONFIG_ADC_BAT_ONOFF,
    CONFIG_ADC_BAT_PARTIAL_PRESSURE,
    CONFIG_ADC_BAT_UPLOAD_INTERVAL_SECONDS,
    CONFIG_META_ONOFF,
    CONFIG_META_UPLOAD_INTERVAL_SECONDS,
    CONFIG_LORA_MODE,//init status: 0 off,1 rx, 2 standby
    CONFIG_LORA_FRE,							
	CONFIG_LORA_TX_OUTPUT_POWER,                     
	CONFIG_LORA_BANDWIDTH,                            
	CONFIG_LORA_SPREADING_FACTOR,                    
	CONFIG_LORA_CODINGRATE,                           
	CONFIG_LORA_PREAMBLE_LENGTH,  
	CONFIG_LORA_FIX_LENGTH_PAYLOAD_ON, 
	CONFIG_LORA_IQ_INVERSION_ON, 
	CONFIG_LORA_RX_TIMEOUT_VALUE,
	CONFIG_LORA_RX_BUF_LEN,
	CONFIG_LORA_TX_TIMEOUT_VALUE,   
	CONFIG_LORA_UPLOAD_INTERVAL_SECONDS,
    CONFIG_UART_ONOFF,  
    CONFIG_UART_UPLOAD_INTERVAL_SECONDS,  
    CONFIG_UART_BAUD_RATE,  
    CONFIG_UART_TX_PIN,  
    CONFIG_UART_RX_PIN,  
    CONFIG_GPIO_ONOFF,  
    CONFIG_GPIO_UPLOAD_INTERVAL_SECONDS,  
	CONFIG_GPIO_ESP_4, 
	CONFIG_GPIO_ESP_5, 
	CONFIG_GPIO_ESP_6, 
	CONFIG_GPIO_ESP_7, 
	CONFIG_GPIO_ESP_15, 
	CONFIG_GPIO_ESP_16, 
	CONFIG_GPIO_ESP_17, 
	CONFIG_GPIO_ESP_18, 
	CONFIG_GPIO_ESP_8, 
	CONFIG_GPIO_ESP_3, 
	CONFIG_GPIO_ESP_46, 
	CONFIG_GPIO_ESP_9, 
	CONFIG_GPIO_ESP_10, 
	CONFIG_GPIO_ESP_11, 
	CONFIG_GPIO_ESP_14, 
	CONFIG_GPIO_ESP_21, 
	CONFIG_GPIO_ESP_48, 
	CONFIG_GPIO_ESP_45, 
	CONFIG_GPIO_ESP_0, 
	CONFIG_GPIO_ESP_38, 
	CONFIG_GPIO_ESP_39, 
	CONFIG_GPIO_ESP_40, 
	CONFIG_GPIO_ESP_41, 
	CONFIG_GPIO_ESP_42, 
	CONFIG_GPIO_ESP_2, 
	CONFIG_GPIO_ESP_1, 
	CONFIG_GPIO_EXT_IO7,
	CONFIG_GPIO_EXT_OC1,
	CONFIG_GPIO_EXT_OC5,
	CONFIG_GPIO_EXT_OC6,
	CONFIG_GPIO_EXT_OC9,
	CONFIG_GPIO_EXT_OC10,
	CONFIG_GPIO_EXT_OC11,
	CONFIG_GPIO_EXT_OC12,
	CONFIG_GPIO_EXT_OC13,
	CONFIG_GPIO_EXT_OC14,
	CONFIG_GPIO_EXT_IO0,
	CONFIG_GPIO_EXT_IO1,
	CONFIG_GPIO_EXT_IO2,
	CONFIG_GPIO_EXT_IO3,
    CONFIG_4G_ONOFF,  
    CONFIG_4G_APN,
    CONFIG_4G_AUTH_NEED,
    CONFIG_4G_AUTH_TYPE,
    CONFIG_4G_AUTH_USER,
    CONFIG_4G_AUTH_PASSWORD,
    CONFIG_4G_CPIN_SECONDS,
    CONFIG_4G_CSQ_SECONDS,
    CONFIG_4G_CREG_SECONDS,
	CONFIG_WIFI_MODE,//0 off, 1 sta, 2 ap 
    CONFIG_WIFI_AP_STA_SSID,
    CONFIG_WIFI_AP_STA_PASSWORD,
    CONFIG_WIFI_AP_CHANNEL,
    CONFIG_WIFI_AP_MAX_CONNECTION,
    CONFIG_WIFI_AP_SSID_HIDDEN,
    CONFIG_WIFI_AP_DNS_UPDATE_SECONDS,
    CONFIG_WIFI_AP_DNS_DEFAULT,
    CONFIG_TM7705_ONOFF,
    CONFIG_TM7705_UPLOAD_INTERVAL_SECONDS,
    CONFIG_TM7705_PINS_ONOFF,
    CONFIG_TM7705_INIT_TIMES,
    CONFIG_TM7705_REG_FREQ,
    CONFIG_TM7705_REG_CONFIG,
    CONFIG_CAMERA_ONOFF,
    CONFIG_CAMERA_UPLOAD_INTERVAL_SECONDS,
    CONFIG_CAMERA_SIZE,
    CONFIG_CAMERA_QUALITY,
    CONFIG_CAMERA_AEC_GAIN,
    CONFIG_W5500_MODE,//0 off, 1 sta dhcpc, 2 sta static ip 3 (todo, w5500 as ap)
    CONFIG_W5500_IP,
    CONFIG_W5500_SUBNET_MASK,
    CONFIG_W5500_GATEWAY,
    CONFIG_W5500_DNS1,
    CONFIG_W5500_DNS2,
    CONFIG_CONFIG_UPLOAD_INTERVAL_SECONDS,
    CONFIG_CONFIG_UPLOAD_ONOFF,
    CONFIG_FORWARD_UPLOAD_INTERVAL_SECONDS,
    CONFIG_FORWARD_UPLOAD_ONOFF,
    CONFIG_AHT20_ONOFF,
    CONFIG_AHT20_UPLOAD_INTERVAL_SECONDS, 
    CONFIG_SPL06_ONOFF,
    CONFIG_SPL06_UPLOAD_INTERVAL_SECONDS, 
    CONFIG_BLE_PASSWORD, 
    CONFIG_RS485_ONOFF,  
    CONFIG_RS485_UPLOAD_INTERVAL_SECONDS,  
    CONFIG_RS485_BAUD_RATE,  
    CONFIG_RS485_TX_PIN,  
    CONFIG_RS485_RX_PIN,  
    CONFIG_PIR_MODE,// same as gpio_int_type_t
    CONFIG_PIR_UPLOAD_INTERVAL_SECONDS,  
    CONFIG_PIR_VALID_TIMES,
    CONFIG_PIR_NOT_REPEAT_UPLOAD_SECONDS,
    CONFIG_PIR_GPIO_NUM,
    CONFIG_PIR_ACTION, 
    CONFIG_PIR_ACTION_TIMES,
    CONFIG_PIR_ACTION_INTERVAL_SECONDS,
    CONFIG_THERMAL_ONOFF,
    CONFIG_THERMAL_UPLOAD_INTERVAL_SECONDS,   
    CONFIG_THERMAL_EMISSIVITY,
    CONFIG_THERMAL_SHIFT,
    CONFIG_ULTRASONIC_ONOFF,
    CONFIG_ULTRASONIC_UPLOAD_INTERVAL_SECONDS,   
    CONFIG_ULTRASONIC_PIN_TRIG,
    CONFIG_ULTRASONIC_PIN_ECHO,
    CONFIG_ULTRASONIC_MEASURE_MILLI_SECONDS,  
    CONFIG_ULTRASONIC_COMPENSATION_MODE,//0:none, 1:pcb based air pressure, temperature, and humidity, 2:fixed 
    CONFIG_ULTRASONIC_COMPENSATION_VALUE,//value - 50 
    CONFIG_ULTRASONIC_TRIGGER_TIMES,
    CONFIG_ULTRASONIC_TRIGGER_MIN,
    CONFIG_ULTRASONIC_TRIGGER_MAX,
    CONFIG_NAME_MAX
} config_name;

/**
 * 以下枚举定义模块，设备可以有很多模块，也可以增减，但系统核心模块如下所示，不能修改。
 * 这里模块的ID值要和云端系统中ModuleDefine.java定义的模块ID一样。如果用户需要增加一个外设传感器，就可以这里添加一个模块。
 *
 * The following enumeration defines modules. Devices can have many modules and they can be added or removed, 
 * but the core system modules listed below cannot be modified.
 * The module ID values here must match those defined in ModuleDefine.java in the cloud system. 
 * If a user needs to add an external sensor, they can add a module here.
 */


typedef enum {
	//The system-defined enumeration values, which cannot be modified.
	MODULE_GENERAL=0,
	MODULE_CLOUD_LOG=1,
	//Below are the user-defined enumeration values, which can be added to but cannot have their numerical values explicitly set.
    MODULE_WIFI=2,
    MODULE_W5500=3,
    MODULE_4G=4,
    MODULE_BLE=5,
    MODULE_TM7705=6,
    MODULE_CAMERA=7,
    MODULE_LORA=8,
    MODULE_UART=9,
    MODULE_SPI=10,
    MODULE_I2C=11,
    MODULE_ADC_BAT=12,
    MODULE_META=13,
    MODULE_GPIO=14,
    MODULE_CONFIG=15,
    MODULE_FORWARD=16,
    MODULE_AHT20=17,
    MODULE_SPL06=18,
    MODULE_RS485=19,
    MODULE_PIR=20,
    MODULE_THERMAL=21,
    MODULE_ULTRASONIC=22,
    MODULE_MAX
} config_module;


/**
 * 设备和云端系统可以双向交互，交互的报文都包含一个COMMAND。
 * 1. 设备周期上报数据
 * 在app_main.c中可以通过UTIL_TASK_ADD定义多个任务，基本上每个任务可以对应一个传感器或者说一个模块。
 * app_main.c中的app_main方法中包含主循环，每一次循环都会执行UTIL_TASK_ADD定义的任务。
 * 枚举command_request中定义了不同的COMMAND, 不同模块对应不同的COMMAND, 模块上报的报文包含COMMAND的值。
 * 设备上报报文到云端系统后，云端系统会根据COMMAND的值，找到对应的模块来处理，COMMAND与模块对应关系在ModuleDefine.java中定义。
 * 最终请求被com.zyc.dc.service.module包下的不同模块处理器处理。
 * 处理后，云端会返回内容，返回报文也包含COMMAND，返回COMMAND定义在枚举command_response内，设备主动上报时返回的内容只会为如下：
 * COMMAND_RSP_COMMON 表示云端收到设备请求，结束。
 * COMMAND_RSP_CONFIG 表示云端收到设备请求，云端需要对设备进行配置，设备会根据报文内容对自身进行配置。
 * COMMAND_RSP_OTA 表示云端收到设备请求，云端需要对设备进行OTA升级，设备会进入OTA升级状态。
 * COMMAND_RSP_ISSUE 表示云端收到设备请求，云端发现有问题。
 * 设备主动上报的每个报文都有唯一的pack_seq，接收到云端的报文内也包含同样的pack_seq，同样的值把发送报文和返回报文关联起来。
 * 同时，接收到报文后，核心系统会回调comm_util.c的response_process函数，因此如果需要特殊处理返回结果可以修改这个函数。
 * 总结一下，设备主动上报报文，云端回复报文，此轮通讯结束。
 *
 * 2. 设备被触发上报数据
 * 设备除了周期执行上报，还可能被外界触发，比如设备外接一个PIR传感器，只要有人经过，PIR传感器会触发设备某个GPIO引脚，引脚被触发后，设备把具体信息上报到云端处理。
 * 目前GPIO, UART, RS485和LORA模块支持触发上报。
 * GPIO，GPIO模块可以对ESP芯片的很多引脚进行配置，如果配置为中断模式，那么引脚电压高低的改变既可以触发中断，触发后信息会被暂存在queue中，gpio_util.c中的void gpio_util_queue(void)函数
 * 会周期查看queue内容进行云端上报。
 * UART，UART模块的RX接收引脚随时可能接收到数据，接收到后会临时存到queue中，uart_util.c的void uart_util_queue(void)函数会周期查看queue内容进行云端上报。
 * RS485，RS485模块的RX接收引脚随时可能接收到数据，接收到后会临时存到queue中，rs485_util.c的void rs485_util_queue(void)函数会周期查看queue内容进行云端上报。
 * LORA， LORA模块有自己的独立线程任务，lora_util.c中static void lora_task(void *pvParameters)函数里面会一直处于监听状态，当接收到LORA报文时立刻上报到云端。
 * 上报报文和返回内容和#1一致。
 *
 * 3. 云端主动下发报文到设备
 * 因设备与云端维持一个TCP/IP连接，因此云端可以随时下发报文。云端通过OperateExecutor.java中public ResultType exeOperate(OperateRequest request, DeviceModel deviceModel)进行报文下发。
 * 云端下发报文的原因多种，可能是用户点击了页面某些操作，比如配置设备，获取设备信息等。也可能是云端API收到第三方系统调用。
 * 云端下发的报文是COMMAND_RSP_OPERATE，此类报文还包含代表本次操作的OPERATE字段，设备接收到COMMAND_RSP_OPERATE类型的报文后，
 * 会交由comm_util.c中的data_send_element *operate_process(uint32_t operate,uint8_t *data_in,uint32_t data_in_len)处理。
 * 根据不同OPERATE字段，用户可以处理其逻辑。
 * 处理完毕，设备会返还COMMAND_REQ_OPERATE报文到云端，云端接收后处理后会再次返回报文来结束本次会话，返回的内容同#1中云端返回内容。
 * 											
 * The device and cloud system can communicate bidirectionally, with all interaction messages containing a COMMAND.
 * 1. Periodic Data Reporting by the Device
 * In app_main.c, multiple tasks can be defined using UTIL_TASK_ADD, where each task can correspond to a sensor or module.
 * The main loop in app_main.c executes tasks defined by UTIL_TASK_ADD in every cycle.
 * Different COMMANDs are defined in the enum command_request, with different modules corresponding to different COMMANDs. Messages reported by modules contain the value of COMMAND.
 * After the device reports a message to the cloud system, the cloud system will find the corresponding module to handle it based on the value of COMMAND. The mapping between COMMAND and module is defined in ModuleDefine.java.
 * Requests are ultimately handled by different module processors under the com.zyc.dc.service.module package.
 * After processing, the cloud returns content, which also includes a COMMAND. The return COMMAND is defined in the enum command_response. When the device actively reports, the returned content can only be as follows:
 * COMMAND_RSP_COMMON indicates that the cloud has received the device's request and ends.
 * COMMAND_RSP_CONFIG indicates that the cloud has received the device's request and needs to configure the device; the device configures itself according to the message content.
 * COMMAND_RSP_OTA indicates that the cloud has received the device's request and needs to perform an OTA upgrade; the device enters the OTA upgrade state.
 * COMMAND_RSP_ISSUE indicates that the cloud has received the device's request and found an issue.
 * Each report message actively sent by the device has a unique pack_seq. The message received from the cloud also contains the same pack_seq, which links the sent message and the returned message with the same value.
 * Upon receiving a message, the core system will callback the response_process function in comm_util.c. Therefore, if special handling of the return result is needed, this function can be modified.
 * This ensures that responses can be appropriately matched with their corresponding requests and allows for customized processing of responses as required.
 * To summarize, after the device actively reports a message and the cloud responds, this round of communication ends.
 *
 * 2. Data Reporting Triggered by External Events
 * Besides periodic reporting, devices can also report data when triggered externally. For example, if the device is connected to a PIR sensor, whenever someone passes by, the PIR sensor triggers a GPIO pin on the device. Once the pin is triggered, the device sends specific information to the cloud for processing.
 * Currently, GPIO, UART, RS485, and LORA modules support triggered reporting.
 * GPIO: The GPIO module can configure many pins on the ESP chip. If configured for interrupt mode, changes in pin voltage can trigger interrupts. Post-trigger information is temporarily stored in a queue, and the function void gpio_util_queue(void) in gpio_util.c periodically checks the queue content for cloud reporting.
 * UART: The RX receive pin of the UART module may receive data at any time. Upon receipt, it is temporarily stored in a queue, and the function void uart_util_queue(void) in uart_util.c periodically checks the queue content for cloud reporting.
 * RS485: Similar to UART, the RX receive pin of the RS485 module may receive data at any time. Upon receipt, it is temporarily stored in a queue, and the function void rs485_util_queue(void) in rs485_util.c periodically checks the queue content for cloud reporting.
 * LORA: The LORA module has its own independent thread task. The static void lora_task(void *pvParameters) in lora_util.c remains in listening status and immediately reports to the cloud upon receiving a LORA message.
 * The reported message and response content are consistent with #1.
 *
 * 3. Cloud System Initiates Message Sending to Device
 * Since the device maintains a TCP/IP connection with the cloud, the cloud can send messages at any time. Messages are sent through public ResultType exeOperate(OperateRequest request, DeviceModel deviceModel) in OperateExecutor.java.
 * There are various reasons for the cloud to send messages, such as user interactions like configuring devices or obtaining device information, or third-party system calls to the cloud API.
 * The cloud sends COMMAND_RSP_OPERATE messages, which include an OPERATE field representing the operation. Upon receiving a COMMAND_RSP_OPERATE type message, the device processes it using data_send_element *operate_process(uint32_t operate,uint8_t *data_in,uint32_t data_in_len) in comm_util.c.
 * Based on different OPERATE fields, users can process their logic.
 * After processing, the device returns a COMMAND_REQ_OPERATE message to the cloud. After receiving and processing, the cloud responds again to end the session, with the response content being the same as described in #1.
 */

 
typedef enum {
	COMMAND_REQ_CAMERA=0,
    COMMAND_REQ_TM7705=1,
    COMMAND_REQ_CONFIG=2,
    COMMAND_REQ_OPERATE=3,
    COMMAND_REQ_META=4,
    COMMAND_REQ_GPIO=5,
    COMMAND_REQ_UART=6,
    COMMAND_REQ_LORA=7,
    COMMAND_REQ_BAT_ADC=8,
    COMMAND_REQ_FORWARD=9,
    COMMAND_REQ_LOG=10,
    COMMAND_REQ_AHT20=11,
    COMMAND_REQ_SPL06=12,
    COMMAND_REQ_RS485=13,
    COMMAND_REQ_PIR=14,
    COMMAND_REQ_THERMAL=15,
    COMMAND_REQ_ULTRASONIC=16,
} command_request;

typedef enum {
	COMMAND_ACROSS_REQ=5000,//仅在云端使用 only used in cloud
	COMMAND_ACROSS_RSP=6000,//仅在云端使用 only used in cloud
	COMMAND_RSP_COMMON=100000,
    COMMAND_RSP_CONFIG=100001,
    COMMAND_RSP_OPERATE=100002,
    COMMAND_RSP_OTA=100003,
    COMMAND_RSP_ISSUE=100004,
    COMMAND_RSP_SLEEP=100005
} command_response;

/**
 * 报文标志位定义。
 * 定义了报文来源及特性相关的标志位。
 * Definitions of message flags.
 * Defines flag bits related to the origin and characteristics of messages.
 */


typedef enum {
    /**
     * 表示报文来自云端。
     * Indicates that the message is from the cloud.
     */
    COMMAND_FLAG_FROM_TCP = (0x01 << 0),

    /**
     * 表示报文来自设备。
     * Indicates that the message is from the device.
     */
    COMMAND_FLAG_FROM_DEVICE = (0x01 << 1),

    /**
     * 表示报文最初来自云端，比如云端下发，设备接收后返回，云端收到返回后再次返回设备确认。
     * Indicates that the message originally came from the cloud, such as when a command is sent from the cloud, received by the device, then returned, and finally confirmed by the cloud after receiving the return.
     */
    COMMAND_FLAG_INIT_FROM_TCP = (0x01 << 2),

    /**
     * 表示报文最初来自设备。
     * Indicates that the message originally came from the device.
     */
    COMMAND_FLAG_INIT_FROM_DEVICE = (0x01 << 3),

    /**
     * 表示报文交互中设备不输出日志，比如云端日志数据上报如果也输出设备日志的话，日志还要被上报，会再次产生日志。
     * Indicates that no logs should be output by the device during message interaction. For example, if cloud log data reporting also outputs device logs, it would create additional logs that need to be reported again.
     */
    COMMAND_FLAG_LOG_NONE = (0x01 << 4),

    /**
     * 云端使用，附带一些额外数据用。
     * Used by the cloud to carry some additional data.
     */
    COMMAND_FLAG_RAW_DATA = (0x01 << 5)
} command_flag;

/**
 * 每次上报数据都要创建一个data_send_container, data_send_container可以通过链表包含多个data_send_element。
 * 每次上报，无论成功与否，创建的data_send_container，data_send_element都会被核心系统free掉，
 * 但data_send_element中data指向的数据是否要被free，需要指定。
 *
 * A new data_send_container must be created for each data report. 
 * A data_send_container can contain multiple data_send_elements through a linked list.
 * Regardless of success or failure, the created data_send_container and data_send_element will be freed by the core system after reporting,
 * However, whether the data pointed to by the 'data' field in data_send_element should be freed needs to be specified.
 */

typedef struct data_send_element {
    /**
     * 1表示发送后data指针内容会被内核系统自动free，0表示不会free需要调用端处理。
     * 1 indicates that the content pointed to by the 'data' pointer will be automatically freed by the kernel system after sending, 
     * 0 indicates that it will not be freed and needs to be handled by the calling end.
     */
    uint8_t need_free;

    /**
     * 要发送的数据。
     * The data to be sent.
     */
    uint8_t *data;

    /**
     * 要发送的数据长度。
     * The length of the data to be sent.
     */
    int data_len;

    /**
     * 链表的下一个节点，NULL表示没有下一个。
     * The next node in the linked list, NULL indicates no next node.
     */
    struct data_send_element *tail;
} data_send_element;

typedef struct data_send_container {
    /**
     * 表示发送过程中是否记录日志。
     * Indicates whether logs should be recorded during sending.
     */
    uint8_t no_log;

    /**
     * 表示本次是否发送日志文件里的内容，一般不用，填写0。
     * Indicates whether to send the contents of the log file this time, generally unused, set to 0.
     */
    uint8_t is_log_file;

    /**
     * 本次发送的报文包含的COMMAND。
     * The COMMAND contained in the message being sent this time.
     */
    command_request command;

    /**
     * 指向data_send_element。
     * Points to the data_send_element.
     */
    data_send_element *element;
} data_send_container;


/**
 * 核心系统会回调这个类型的函数指针，从而在启动时读取用户配置项。
 * The core system will callback this type of function pointer to read user configuration items during startup.
 */
typedef void (*config_callback)(void);

/**
 * 配置内容写到ESP的FLASH上存储，返回0表示失败，返回1表示成功。
 * Writes the configuration content to ESP's FLASH storage, returns 0 for failure and 1 for success.
 */
uint8_t config_write(void);

/**
 * 序列化配置项到buf中，返回buf的总长度，如果buf为NULL，只计算总长度并返回，因此一般先输入NULL调用一次，然后根据返回大小申请buf的内存，然后再调用一次。
 * Serializes the configuration items into buf, returns the total length of buf. If buf is NULL, only calculates and returns the total length. Therefore, it is generally recommended to call once with NULL first, then allocate memory for buf based on the returned size, and call again.
 */
uint32_t config_to_stream(uint8_t *buf);

/**
 * 序列化配置项到buf中，返回buf的总长度，如果buf为NULL，只计算总长度并返回。
 * Serializes the configuration items into buf, returns the total length of buf. If buf is NULL, only calculates and returns the total length.
 */
uint32_t config_to_stream(uint8_t *buf);

/**
 * 序列化配置项，并设置到输入的data_element中，为发送配置内容到云端做准备。
 * Serializes the configuration items and sets them into the input data_element, preparing for sending configuration content to the cloud.
 */
void config_request(data_send_element *data_element);

/**
 * 打印当前配置项内容，prefix为打印前缀。
 * Prints the current configuration item contents, prefix is the print prefix.
 */
void config_print_variable(char *prefix);

/**
 * 返回数值类型的配置项的当前值。
 * Returns the current value of a numeric configuration item.
 */
uint32_t device_config_get_number(config_name name);

/**
 * 无论何种类型的配置项都可以调用此函数，函数按二进制格式设置。返回0表示之前数值和本次设置的相同，1表示设置成功，2表示设置的值不合法。
 * This function can be called for any type of configuration item, setting it in binary format. Returns 0 if the previous value is the same as the new one, 1 for successful setting, and 2 if the value is invalid.
 */
uint8_t device_config_set_binary(config_name name, uint8_t *data, uint32_t data_len);

/**
 * 设置数值类型的配置项。返回0表示之前数值和本次设置的相同，1表示设置成功，2表示设置的值不合法。
 * Sets the value of a numeric configuration item. Returns 0 if the previous value is the same as the new one, 1 for successful setting, and 2 if the value is invalid.
 */
uint8_t device_config_set_number(config_name name, uint32_t value);

/**
 * 返回配置项的当前值，如果是数值类型，value会存放按"%u"格式打印的值，如果打印的字符串超过value_len的长度，按value_len长度截取。如果是字符串类型，value_len为0，则会返回全部字符，因此要保证value的长度能够容纳。如果value_len>0，则会在字符串实际长度和value_len取最小值，然后按最小值返回字符串。
 * Returns the current value of the configuration item. For numeric types, value will store the value formatted as "%u", truncated to value_len if necessary. For string types, if value_len is 0, returns the entire string (ensure value has sufficient length). If value_len > 0, returns the minimum between the actual string length and value_len.
 */
uint32_t device_config_get_string(config_name name, char *value, uint32_t value_len);

/**
 * 返回字符串类型配置项的长度。
 * Returns the length of a string-type configuration item.
 */
uint32_t device_config_get_string_len(config_name name);

/**
 * 设置字符串类型的配置项。返回0表示之前数值和本次设置的相同，1表示设置成功，2表示设置的值不合法。
 * Sets the value of a string-type configuration item. Returns 0 if the previous value is the same as the new one, 1 for successful setting, and 2 if the value is invalid.
 */
uint8_t device_config_set_string(config_name name, char *value, uint32_t value_len);

/**
 * 判断字符串类型的配置项的当前值是否和value以及value_len指定的字符串相等，相等返回1，不等返回0。
 * Checks if the current value of a string-type configuration item matches the specified string (value and value_len). Returns 1 if equal, 0 otherwise.
 */
uint8_t device_config_same_string(config_name name, char *value, uint32_t value_len);

/**
 * 设置用户配置项。
 * Set user configuration items.
 */
void define_user_config(void);

/**
 * 设置配置项最大数目，内核系统按这个数目申请最大空间，不能超过1000个。返回0表示成功，-1表示数值太大，超过了1000，-2表示数值太小，进入了内核配置项范围。
 * Sets the maximum number of configuration items, which the kernel system uses to allocate maximum space. Cannot exceed 1000. Returns 0 for success, -1 if the number is too large (exceeds 1000), and -2 if the number is too small (enters the kernel configuration item range).
 */
int define_config_max(uint32_t max_config_in);

/**
 * 设置数值类型的配置项。module为枚举config_module里的值；idx为枚举config_name里的值；name_in为字符串常量；value_in为配置项默认值；value_len_in为配置项默认值所占字节数；flag为枚举config_flag里的值；type为枚举config_type里的值；range_count表示范围参数个数；...具体的范围参数，如果是一个范围，输入最小值和最大值，如果是枚举，输入每一个可能值。
 * Sets a numeric type configuration item. module is a value from the config_module enum; idx is a value from the config_name enum; name_in is a string constant; value_in is the default value of the configuration item; value_len_in is the byte size of the default value; flag is a value from the config_flag enum; type is a value from the config_type enum; range_count indicates the number of range parameters; ... specific range parameters, if it's a range, input minimum and maximum values, if it's an enumeration, input each possible value.
 */
int define_number_variable(uint32_t module, uint32_t idx, char *name_in, uint32_t value_in, uint32_t value_len_in, uint8_t flag, config_type type, int range_count, ...);

/**
 * 设置字符串类型的配置项。module为枚举config_module里的值；idx为枚举config_name里的值；name_in为字符串常量；value_in为配置项默认值；value_len_in为配置项默认值所占字节数；flag为枚举config_flag里的值；type为枚举config_type里的值；min_len配置内容允许的最小字节数；max_len配置内容允许的最大字节数。
 * Sets a string type configuration item. module is a value from the config_module enum; idx is a value from the config_name enum; name_in is a string constant; value_in is the default value of the configuration item; value_len_in is the byte size of the default value; flag is a value from the config_flag enum; type is a value from the config_type enum; min_len is the minimum byte size allowed for the content; max_len is the maximum byte size allowed for the content.
 */
int define_string_variable(uint32_t module, uint32_t idx, char *name_in, char *value_in, uint32_t value_len_in, uint8_t flag, config_type type, uint32_t min_len, uint32_t max_len);

void variable_set(void *dst,const void *src, uint32_t len);
#ifdef __cplusplus
}
#endif
