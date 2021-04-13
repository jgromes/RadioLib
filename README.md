# RadioLib ![Build Status](https://github.com/jgromes/RadioLib/workflows/CI/badge.svg)

### _One radio library to rule them all!_

## Universal wireless communication library for Arduino

## See the [Wiki](https://github.com/jgromes/RadioLib/wiki) for further information. See the [GitHub Pages](https://jgromes.github.io/RadioLib) for detailed and up-to-date API reference.

RadioLib allows its users to integrate all sorts of different wireless communication modules, protocols and even digital modes into a single consistent system.
Want to add a Bluetooth interface to your LoRa network? Sure thing! Do you just want to go really old-school and play around with radio teletype, slow-scan TV, or even Hellschreiber using nothing but a cheap radio module? Why not!

RadioLib was originally created as a driver for [__RadioShield__](https://github.com/jgromes/RadioShield), but it can be used to control as many different wireless modules as you like - or at least as many as your Arduino can handle!

### Supported modules:
* __CC1101__ FSK radio module
* __ESP8266__ WiFi module
* __HC05__ Bluetooth module
* __JDY08__ BLE module
* __nRF24L01__ 2.4 GHz module
* __RF69__ FSK/OOK radio module
* __RFM2x__ series FSK modules (RFM22, RM23)
* __RFM9x__ series LoRa modules (RFM95, RM96, RFM97, RFM98)
* __Si443x__ series FSK modules (Si4430, Si4431, Si4432)
* __SX126x__ series LoRa modules (SX1261, SX1262, SX1268)
* __SX127x__ series LoRa modules (SX1272, SX1273, SX1276, SX1277, SX1278, SX1279)
* __SX128x__ series LoRa/GFSK/BLE/FLRC modules (SX1280, SX1281, SX1282)
* __SX1231__ FSK/OOK radio module
* __XBee__ modules (S2B)

### Supported protocols and digital modes:
* __MQTT__ for modules:  
ESP8266
* __HTTP__ for modules:  
ESP8266
* __AX.25__ using 2-FSK or AFSK for modules:  
SX127x, RFM9x, SX126x, RF69, SX1231, CC1101, RFM2x and Si443x
* [__RTTY__](https://www.sigidwiki.com/wiki/RTTY) using 2-FSK or AFSK for modules:  
SX127x, RFM9x, SX126x, RF69, SX1231, CC1101, nRF24L01, RFM2x, Si443x and SX128x
* [__Morse Code__](https://www.sigidwiki.com/wiki/Morse_Code_(CW)) using 2-FSK or AFSK for modules:  
SX127x, RFM9x, SX126x, RF69, SX1231, CC1101, nRF24L01, RFM2x, Si443x and SX128x
* [__SSTV__](https://www.sigidwiki.com/wiki/SSTV) using 2-FSK or AFSK for modules:  
SX127x, RFM9x, SX126x, RF69, SX1231, CC1101, RFM2x and Si443x
* [__Hellschreiber__](https://www.sigidwiki.com/wiki/Hellschreiber) using 2-FSK or AFSK for modules:  
SX127x, RFM9x, SX126x, RF69, SX1231, CC1101, nRF24L01, RFM2x, Si443x and SX128x

### Supported platforms:
* __Arduino__  
  * [__AVR__](https://github.com/arduino/ArduinoCore-avr) - Arduino Uno, Mega, Leonardo, Pro Mini, Nano etc.
  * [__mbed__](https://github.com/arduino/ArduinoCore-mbed) - Arduino Nano 33 BLE and Arduino Portenta H7
  * [__megaAVR__](https://github.com/arduino/ArduinoCore-megaavr) - Arduino Uno WiFi Rev.2 and Nano Every
  * [__SAM__](https://github.com/arduino/ArduinoCore-sam) - Arduino Due
  * [__SAMD__](https://github.com/arduino/ArduinoCore-samd) - Arduino Zero, MKR boards, M0 Pro etc.

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

The list above is by no means exhaustive. Most of RadioLib code is independent of the used platform, so as long as your board is running some Arduino-compatible core, RadioLib should work. Compilation of all examples is tested for all platforms prior to releasing new version.

### In development:
* __SIM800C__ GSM module
* __LoRaWAN__ protocol for SX127x, RFM9x and SX126x modules
* __APRS__ protocol for all the modules that can transmit AX.25
* ___and more!___

## Frequently Asked Questions

### Where should I start?
First of all, take a look at the [examples](https://github.com/jgromes/RadioLib/tree/master/examples) and the [Wiki](https://github.com/jgromes/RadioLib/wiki) - especially the [Basics](https://github.com/jgromes/RadioLib/wiki/Basics) page. There's a lot of useful information over there. If something isn't working as expected, try searching the [issues](https://github.com/jgromes/RadioLib/issues/).

### Help, my module isn't working!
The fastest way to get help is by creating an [issue](https://github.com/jgromes/RadioLib/issues/new/choose) using the appropriate template. It is also highly recommended to try running the examples first - their functionality is tested from time to time and they should work. Finally, RadioLib is still under development, which means that sometimes, backwards-incompatible changes might be introduced. Though these are kept at minimum, sometimes it is unavoidable. You can check the [release changelog](https://github.com/jgromes/RadioLib/releases) to find out if there's been such a major change recently.

### RadioLib doesn't support my module! What should I do?
Start by creating new issue (if it doesn't exist yet). If you have some experience with Arduino and C/C++ in general, you can try to add the support yourself! Use the template files in `/extras/` folder to get started. This is by far the fastest way to implement new modules into RadioLib, since I can't be working on everything all the time. If you don't trust your programming skills enough to have a go at it yourself, don't worry. I will try to implement all requested modules, but it will take me a while.
