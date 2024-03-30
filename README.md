# Universal io-homecontrol library for embedded devices

<!-- TODO: Register pio and espressif
RadioLib_IoHc ![Build Status](https://github.com/velocet/RadioLib_IoHc/workflows/CI/badge.svg) [![PlatformIO Registry](https://badges.registry.platformio.org/packages/velocet/library/radiolib_iohc.svg)](https://registry.platformio.org/libraries/velocet/radiolib_iohc) [![Component Registry](https://components.espressif.com/components/velocet/radiolib_iohc/badge.svg)](https://components.espressif.com/components/velocet/radiolib_iohc)
-->
Fork of the latest RadioLib with an additional io-homecontrol layer to abstract the configuration of the modules.

RadioLib_IoHc allows to integrate different radio modules which support FSK mode of operation in the 868MHz range into a single consistent system.

Natively supports Arduino, but can run in non-Arduino environments as well!

## Supported modules & Arduino platforms:

This list is by no means exhaustive - the code is independent of the used platform!
In addition, the library includes an internal hardware abstraction layer, which allows it to be easily ported even to non-Arduino environments.

### Modules

* __CC1101__ FSK module
* __LLCC68__ FSK module (untested)
* __LR1121__ FSK module (unimplemented)
* __RF69__ series FSK modules
* __RFM2x__ series FSK modules (RFM22(B), RFM23(B))
* __RFM9x__ series FSK modules (RFM95(W), RFM97(W))
* __Si443x__ series FSK modules (Si4431, Si4432)
* __STM32WL__ FSK module/microcontroller
* __SX126x__ series FSK modules (SX1261, SX1262)
* __SX127x__ series FSK modules (SX1272, SX1273, SX1276, SX1277, SX1279)
* __SX123x__ series FSK modules (SX1231, SX1233, SX1235)

### Arduino platforms:

* __Arduino__  
  * [__AVR__](https://github.com/arduino/ArduinoCore-avr) - Arduino Uno, Mega, Leonardo, Pro Mini, Nano etc.
  * [__mbed__](https://github.com/arduino/ArduinoCore-mbed) - Arduino Nano 33 BLE and Arduino Portenta H7
  * [__megaAVR__](https://github.com/arduino/ArduinoCore-megaavr) - Arduino Uno WiFi Rev.2 and Nano Every
  * [__SAM__](https://github.com/arduino/ArduinoCore-sam) - Arduino Due
  * [__SAMD__](https://github.com/arduino/ArduinoCore-samd) - Arduino Zero, MKR boards, M0 Pro etc.
  * [__Renesas__](https://github.com/arduino/ArduinoCore-renesas) - Arduino Uno R4

* __Adafruit__
  * [__SAMD__](https://github.com/adafruit/ArduinoCore-samd) - Adafruit Feather M0 and M4 boards (Feather, Metro, Gemma, Trinket etc.)
  * [__nRF52__](https://github.com/adafruit/Adafruit_nRF52_Arduino) - Adafruit Feather nRF528x, Bluefruit and CLUE

* __Espressif__
  * [__ESP32__](https://github.com/espressif/arduino-esp32) - ESP32-based boards
  * [__ESP8266__](https://github.com/esp8266/Arduino) - ESP8266-based boards

* __Intel__
  * [__Curie__](https://github.com/arduino/ArduinoCore-arc32) - Arduino 101

* __SparkFun__
  * [__Apollo3__](https://github.com/sparkfun/Arduino_Apollo3) - Sparkfun Artemis Redboard

* __ST Microelectronics__
  * [__STM32__ (official core)](https://github.com/stm32duino/Arduino_Core_STM32) - STM32 Nucleo, Discovery, Maple, BluePill, BlackPill etc.
  * [__STM32__ (unofficial core)](https://github.com/rogerclarkmelbourne/Arduino_STM32) - STM32F1 and STM32F4-based boards

* __MCUdude__
  * [__MegaCoreX__](https://github.com/MCUdude/MegaCoreX) - megaAVR-0 series (ATmega4809, ATmega3209 etc.)
  * [__MegaCore__](https://github.com/MCUdude/MegaCore) - AVR (ATmega1281, ATmega640 etc.)

* __Raspberry Pi__
  * [__RP2040__ (official core)](https://github.com/arduino/ArduinoCore-mbed) - Raspberry Pi Pico and Arduino Nano RP2040 Connect
  * [__RP2040__ (unofficial core)](https://github.com/earlephilhower/arduino-pico) - Raspberry Pi Pico/RP2040-based boards
  * [__Raspberry Pi__](https://github.com/me-no-dev/RasPiArduino) - Arduino framework for RaspberryPI

* __Heltec__
  * [__CubeCell__](https://github.com/HelTecAutomation/CubeCell-Arduino) - ASR650X series (CubeCell-Board, CubeCell-Capsule, CubeCell-Module etc.)

* __PJRC__
  * [__Teensy__](https://github.com/PaulStoffregen/cores) - Teensy 2.x, 3.x and 4.x boards
