# RadioLib [![Build Status](https://travis-ci.org/jgromes/RadioLib.svg?branch=master)](https://travis-ci.org/jgromes/RadioLib)

### _One radio library to rule them all!_

## Universal wireless communication library for Arduino

## See the [Wiki](https://github.com/jgromes/RadioLib/wiki) for further information. See the [GitHub Pages](https://jgromes.github.io/RadioLib) for detailed and up-to-date API reference.

RadioLib allows its users to integrate all sorts of different wireless communication modules into a single consistent system.
Want to add a Bluetooth interface to your ZigBee network? Sure thing! Need to connect LoRa network to the Internet with a GSM module? RadioLib has got your back!

RadioLib was originally created as a driver for [__RadioShield__](https://github.com/jgromes/RadioShield), but it can be used to control as many different wireless modules as you like - or at least as many as your Arduino can handle!

### Supported modules:
* __CC1101__ FSK radio module
* __ESP8266__ WiFi module
* __HC05__ Bluetooth module
* __JDY08__ BLE module
* __nRF24L01__ 2.4 GHz module
* __RF69__ FSK/OOK radio module
* __RFM9x__ series LoRa modules (RFM95, RM96, RFM97, RFM98)
* __SX127x__ series LoRa modules (SX1272, SX1273, SX1276, SX1277, SX1278, SX1279)
* __SX126x__ series LoRa modules (SX1261, SX1262, SX1268)
* __SX1231__ FSK/OOK radio module
* __XBee__ modules (S2B)

### Supported protocols:
* __MQTT__ for modules: ESP8266
* __HTTP__ for modules: ESP8266
* __RTTY__ for modules: SX127x, RFM9x, SX126x, RF69, SX1231, CC1101 and nRF24L01
* __Morse Code__ for modules: SX127x, RFM9x, SX126x, RF69, SX1231, CC1101 and nRF24L01

### Supported platforms:
* __AVR__ - tested on Uno, Mega and Leonardo
* __ESP8266__ - tested NodeMCU and Wemos D1
* __ESP32__ - tested on ESP-WROOM-32
* __STM32__ - tested on Nucleo L452RE-P
* __SAMD__ - Arduino Zero, Arduino MKR boards, M0 Pro etc.
* __SAM__ - Arduino Due
* __nRF52__ - Adafruit Bluefruit Feather etc.
* __Intel Curie__ - Arduino 101
* __megaAVR__ - Arduino Uno WiFi Rev.2 etc.

The list above is by no means exhaustive. Most of RadioLib code is independent of the used platform, so as long as your board is running some Arduino-compatible core, RadioLib should work.

### In development:
* __SIM800C__ GSM module
* __LoRaWAN__ protocol for SX127x, RFM9x and SX126x modules
* ___and more!___

## Frequently Asked Questions

### Where should I start?
First of all, take a look at the [examples](https://github.com/jgromes/RadioLib/tree/master/examples) and the [Wiki](https://github.com/jgromes/RadioLib/wiki) - especially the [Basics](https://github.com/jgromes/RadioLib/wiki/Basics) page. There's a lot of useful information over there. Also, you should check out [RadioShield](https://github.com/jgromes/RadioShield) - open source Arduino shield that will allow you to easily connect any two wireless modules supported by RadioLib!

### Help, my module isn't working!
The fastest way to get help is by creating an [issue](https://github.com/jgromes/RadioLib/issues/new?assignees=&labels=&template=bug_report.md&title=) using the appropriate template. It is also highly recommended to try running the examples first - their functionality is tested from time to time and they should work. Finally, RadioLib is still under development, which means that sometimes, backwards-incompatible changes might be introduced. Though these are kept at minimum, sometimes it is unavoidable. You can check the [release changelog](https://github.com/jgromes/RadioLib/releases) to find out if there's been such a major change recently.

### RadioLib doesn't support my module! What should I do?
Start by creating new issue (if it doesn't exist yet). If you have some experience with Arduino and C/C++ in general, you can try to add the support yourself! Use the template files in `/extras/` folder to get started. This is by far the fastest way to implement new modules into RadioLib, since I can't be working on everything all the time. If you don't trust your programming skills enough to have a go at it yourself, don't worry. I will try to implement all requested modules, but it will take me a while.
