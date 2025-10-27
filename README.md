# ESPnRF24 Jammer
A low-power 2,4GHz wireless jammer based on `ESP32` and `nRF24LO1+PA+LNA`

## Features

- A web interface for control and settings
- Multiple jamming modes:
  - WiFi (2.4 GHz)
  - Bluetooth, BLE
  - Zigbee
  - Drones
  - Custom frequency ranges
- Configurable `TX` power in the interface
- A captive portal to redirect user to the interface automatically
- `SSD1306` OLED display support for basic status
- Various configuration options
- And more

## Hardware Requirements

For this project, i have used my cheap `esp32s3-devkitc-1`

However any other `ESP32` board with these specs will work too:
  - Dual-core chip
  - Two usable SPI buses
  - 1MB or more `ROM` + room for SPIFFS

For the componenets:
  - 2x `nRF24L01+PA+LNA` modules
  - 2x `100uF` electrolytic capacitors of any voltage
  - Optionally, `SSD1306` OLED display (preferably `128x64`)
  - Prototype/Bread board, and some wiring

## Assembling

- Solder the capacitors across the `VCC` and `GND` pins of each `nRF24` module
- Wire the display/radio modules to your `ESP32` board according to the pinout
and your pin configuration in `include/options.h`

## Building

1. Install VSCode and PlatformIO
2. Edit preferences in `include/options.h`
3. If using a different board, port it into `platformio.ini`
4. Upload the code, and the SPIFFS filesystem image via platform tasks

## WebUI
<img width="1080" height="512" alt="WebUI" src="https://github.com/user-attachments/assets/8e401b94-a49b-4b02-bb4f-1be372aa586a" />

# Credits
This code is my refactor of https://github.com/W0rthlessS0ul/nRF24_jammer with extra features

> [!WARNING] 
> This project is for educational and research purposes only. The use of radio frequency jammers is illegal in many countries. You are responsible for complying with all local laws. Misuse can result in severe legal penalties, including fines and imprisonment. The creators are not liable for any misuse.
