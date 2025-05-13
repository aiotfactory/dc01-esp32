# Project Introduction

Our company has over 20 years of experience in embedded product development, manufacturing, and IoT platform construction. Based on the needs of many customers, we have developed this open-hardware device.

In most cases, customers need to validate their project ideas before starting a full-scale implementation. To support rapid and low-cost validation, we have integrated as many sensors, connectivity modules, power options, and other components into one single product. We also provide convenient online programming and flashing methods so that customers can quickly verify their ideas at minimal cost.

Of course, when customers are ready for mass production, unnecessary components should be removed or optimized to precisely meet their specific requirements — not too many, not too few — achieving the lowest possible cost. Customers can either customize the design themselves or commission our team to handle the design and production.

<img src="docs/pcb02.jpg" alt="PCB Image" width="300"/>

## Sensors

Temperature, humidity, barometric pressure, thermal imaging, camera (with wide-angle, fisheye, infrared, and other lenses), PIR, ultrasonic distance measurement, light radiation intensity.

## Connectivity

Wi-Fi, Ethernet, 4G Cat1 (global and China Mainland standards), BLE, LoRa point-to-point communication (with dedicated PA amplifier, up to 10 km range).

## Interfaces

I2C, I2S, UART, SPI, RS485, GPIO.

## ADC

1 x 12-bit and 2 x 16-bit ADC channels.

## Moving Parts

2-axis gimbal.

## Power Options

AC adapter, rechargeable battery, disposable battery, dual power supply (adapter + battery).

---

## Installation Steps

### Preparation Tools

Install Git from: https://git-scm.com/downloads (choose the version suitable for your OS).  
For convenience, add Git to the system PATH environment variable after installation.

Install Python from: https://www.python.org/ (choose the version suitable for your OS).  
For convenience, add both `python` and `pip` to the system PATH environment variable after installation.

---

### Download ESP-IDF

The following commands must be executed in **Git Bash**. PowerShell or Windows CMD is **not supported**.

Git supports replacing repository URLs using commands similar to the ones below:

```
:: Enter or create an installation directory (assuming your working directory is D:\project\datacollection\temp1)
cd /d/project/datacollection/temp1

:: Clone the esp-gitee-tools mirror utility from Gitee
git clone https://gitee.com/EspressifSystems/esp-gitee-tools.git

:: Set up Git mirror replacement rule:
:: All requests to https://github.com/espressif/esp-idf
:: will be automatically replaced with https://jihulab.com/esp-mirror/espressif/esp-idf
:: This helps speed up cloning by using a domestic mirror
git config --global url.https://jihulab.com/esp-mirror/espressif/esp-idf.insteadOf https://github.com/espressif/esp-idf

:: Run the jihu-mirror.sh script to activate the mirror configuration
./esp-gitee-tools/jihu-mirror.sh set

:: Clone ESP-IDF source code using the mirror (automatically uses the configured mirror)
:: Switch to branch v5.3.2
:: --recursive means all submodules will also be cloned
git clone -b v5.3.2 --recursive https://github.com/espressif/esp-idf.git

:: Go to the esp-gitee-tools directory to prepare for running the installation script
cd esp-gitee-tools

:: Run install.sh and specify the ESP-IDF path to install the required environment
:: This script installs the toolchain and dependencies needed for ESP-IDF
./install.sh /d/project/datacollection/temp1/esp-idf
```

You can use the command `./jihu-mirror.sh unset` to revert and stop using the mirror URL.

---

### Configure ESP-IDF

Execute the following in a **Windows Command Prompt** (Win+R, type `cmd.exe`):

```
:: Navigate to the IDF directory
cd D:\project\datacollection\temp1\esp-idf

:: Set local pip source
pip config set global.index-url http://mirrors.aliyun.com/pypi/simple
pip config set global.trusted-host mirrors.aliyun.com

:: Install dependencies
install.bat

:: Export environment variables
export.bat

:: Modify IDF so that SPI default level is 1
Edit D:\project\datacollection\temp1\esp-idf\components\hal\spi_hal.c, change 0 to 1, then save.
#if SPI_LL_MOSI_FREE_LEVEL 
    // Change default data line level to low which same as esp32
    spi_ll_set_mosi_free_level(hw, 1);  // changed to 1, originally 0
#endif
```

---

### Build the Project

Execute the following in a **Windows Command Prompt** (Win+R, type `cmd.exe`):

```
:: Navigate to the working directory
cd D:\project\datacollection\temp1

:: Run export.bat from IDF
.\esp-idf\export.bat

:: Clone the project files
git clone https://gitee.com/aiotfactory/dc01-esp32.git

:: Build the project
cd dc01-esp32
idf.py build
```

---

### Flashing

Do not exit the terminal after building. If you have exited, re-run `export.bat` and navigate back to the project directory (`dc01-esp32`) before proceeding.

Plug the programmer into your computer's USB port. On first use, the driver will install automatically. After installation, check the Device Manager to confirm successful installation and note the COM port number. The COM port may change each time the device is reconnected, so always double-check it.

<img src="docs/burn01.jpg" alt="Flashing Adapter" width="300"/>

In the image below, the red circle labeled "2" is the flashing mode toggle button. Press and hold it before powering on the device. Label "1" is the power port, "3" is where the wiring connects, make sure the wiring order is correct. If loose, press firmly while starting the flashing process. Label "4" connects to the PC.

<img src="docs/pcb01.jpg" alt="PCB Flashing" width="400"/>

```
:: First, press and hold the flash button, then power on the device.
:: Then execute the following command in the project root directory.
:: If failed, try several times.
idf.py -p COM42 flash monitor
```