# KiteLib

### _One library to rule them all!_

## Universal wireless communication library for Arduino

KiteLib allows its users to integrate all sorts of different wireless communication modules into a single consistent system.
Want to add a Bluetooth interface to your ZigBee network? Sure thing! Need to connect LoRa network to the Internet with a GSM module? KiteLib has got your back!

KiteLib was originally created as a driver for the [__KITE shield__](https://github.com/jgromes/KiteShield), but it can be used to control as many different wireless modules as you like - or at least as many as your Arduino can handle!


### Supported modules:
* __ES8266__ WiFi module
* __HC05__ Bluetooth module
* __JDY08__ BLE module
* __RF69__ FSK/OOK radio module
* __SX127x__ series LoRa modules (SX1272/73/76/77/78/79)
* __SX1231__ FSK/OOK radio module
* __XBee__ modules (S2B)

### Supported protocols:
* __MQTT__ on modules: ESP8266
* __HTTP__ on modules: ESP8266
* __RTTY__ on modules: SX1272/73/76/77/78/79

### In development:
* __SIM800C__ GSM module
* __CC1101__ FSK module
* __nRF24L01__ 2.4 GHz module
* __LoRaWAN__ protocol on SX127x modules
* ___and more!___

## Frequently Asked Questions

### Where should I start?
First of all, take a look at the [examples](https://github.com/jgromes/KiteLib/tree/master/examples) and the [Wiki](https://github.com/jgromes/KiteLib/wiki). There's a lot of useful information over there. Also, you should check out [KITE shield](https://github.com/jgromes/KiteShield) - open source Arduino shield that will allow you to easily connect any two wireless modules supported by KiteLib!

### KiteLib doesn't support my module! What should I do?
Start by creating new issue (if it doesn't exist yet). If you have some experience with Arduino and C/C++ in general, you can try to add the support yourself! Use the template files in `/extras/` folder to get started. This is by far the fastest way to implement new modules into KiteLib, since I can't be working on everything all the time. If you don't trust your programming skills enough to have a go at it yourself, don't worry. I will try to implement all requested modules, but it will take me a while.
