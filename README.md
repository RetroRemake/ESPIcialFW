# ESPIcialFW
Firmware for Enhanced VERA with ESP32 and SPI

![VERA ESPIcial 3D Render](../Assets/VERA_ESPIcial_Render1.png)

## Overview

The VERA ESPIcial is the VERA retro video adapter with additional features for makers
and retro enthusiasts to add to their own designs.

Key features of the ESPIcial include:

* Easy update interface for both ESP32 and FPGA through USB C connector
* Supports all video and audio features of the original VERA
  * 640x480 @60Hz Resolution
  * Multiple video output modes
    * VGA
	* 15KHz RGB with separate or composite sync
	* 480i NTSC-compatible CVBS Color Composite
	* 480i S-Video
	* "240p" non-interlaced NTSC compatible video
  * 4092 colors (4 bits per color)
  * Tiled and Bitmap graphics
  * Up to 128 Sprites
* Backwards compatible with original VERA 8-bit bus interface for Homebrew Retro Computers
* SPI interface for adding video to modern microcontrollers
* Power can be supplied through the USB type C connector or +5V on bus connector
* Can operate as its own single-board computer
  * Onboard ESP32-S3-WROOM-1-N4R8 Module 
    * Dual 32-bit Xtensa LX7 Cores at 240 MHz
	* Built-in 2.4GHz 802.11 b/g/n WiFi and Antenna
	* Built-in Bluetooth 5 LE
	* 4MB Flash
	* 8MB RAM
  * USB type A port for keyboard (possibly keyboard + Mouse)
  * 3.5mm line level headphone jack for audio
  * Micro SD storage accessible to ESP32 or FPGA
  * I2C Interface
  * 6 user GPIO pins

## Getting Started

### Firmware Environment

Currently the firmware uses the Arduino IDE with the Espressif ESP32 board plugin.
Once the ESP32 board is added to Arduino, use these settings for building the firmware:

1. Board type set to "ESP32 Arduino"/"ESP32S3 Dev Module"
2. Upload Speed is your choice, I use 921600 which seems to work well
3. Upload Mode "UART0 / Hardware CDC" (this was the default in my installation)
4. CPU Frequency "240MHz (WiFi)" (default)
5. Flash Mode "QIO 80MHz" (default)
6. Flash Size "4MB (32Mb)" (default)

### Building Firmware

Press the checkmark button on the Arduino IDE to build the firmware. This may take a few
minutes to complete.

### Updating the ESP32 Firmware

The ESP32 Firmware can be updated directly from the Arduino IDE by pressing the "Upload" button.

### Updating the FPGA Firmware

The ESP32 Firmware contains an interface that allows the FPGA bitstream to be updated over
the USB C serial interface. A Python script, `espi_update_fpga.py` is included here to facilitate
this. The script requires PySerial to communicate with the ESP32 firmware.

Obtain a VERA firmware and execute the following:

`python espi_update_fpga.py /dev/ttyS3 ~/vera.bin`

Replace `/dev/ttyS3` with the path to the serial port device on your computer (could be `COM3` on Windows) and replace `~/vera.bin` with the path to the VERA bitstream.