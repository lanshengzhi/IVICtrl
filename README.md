# ESP32 Development environment setup

## Install CP210x or CH340 driver

USB to Serial Port Chip. 

Confirm which chip is used in the development board, then download the corresponding drivers.  

[CP210x driver download.](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads)  
[CH340 driver download](http://www.wch.cn/downloads/CH341SER_EXE.html), Usually, Windows has these drivers installed by default. Just connect the USB cable, open Windows' Device Manager, and check it out.  

## Install vscode

- Install vscode
- Install vscode extensions
    - C/C++
    - PlatformIO IDE

## PlatformIO

### Project setup

- PlatformIO->PIO Home->Open->New Project
- Project name: Your project's name.
- Board: `Espressif ESP32 Dev Module`, this my hardward board.
- Framework: My alternative is `Arduino`.

### Setting the serial port baud rate

#### PC side

In `platformio.ini`, at the bottom line add `monitor_speed = 921600`. 

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 921600
```

#### Device side

In `main.cpp`, add `Serial.begin(921600); `.

```cpp
 void setup() {
    // put your setup code here, to run once:
    Serial.begin(921600); 
}
```
