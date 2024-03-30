# LoRaWAN examples
RadioLib LoRaWAN v1.1 examples.

* [LoRaWAN_Starter](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Starter): this is the recommended entry point for new users. Please read the [`notes`](https://github.com/jgromes/RadioLib/blob/master/examples/LoRaWAN/LoRaWAN_Starter/notes.md) that come with this example to learn more about LoRaWAN and how to use it in RadioLib!
* [LoRaWAN_Reference](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Reference): this sketch showcases most of the available API for LoRaWAN in RadioLib. Be frightened by the possibilities! It is recommended you have read all the [`notes`](https://github.com/jgromes/RadioLib/blob/master/examples/LoRaWAN/LoRaWAN_Starter/notes.md) for the Starter sketch first, as well as the [Learn section on The Things Network](https://www.thethingsnetwork.org/docs/lorawan/)!
* [LoRaWAN_ABP](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_ABP): if you wish to use ABP instead of OTAA (but why?), this example shows how you can do this using RadioLib.

---

> [!WARNING]
> These examples do not fully comply with LoRaWAN v1.1: for that, persistent storage is necessary. As the implementation of persistent storage differs between different platforms, these are not given here, but in a separate repository, see below:

## RadioLib persistence
In [this repository](https://github.com/radiolib-org/radiolib-persistence), examples are provided that do comply with the required persistence of certain parameters for LoRaWAN v1.1. Examples are (or will become) available for some of the most popular platforms. **These examples assume you have successfully used the Starter sketch and understood (most of) the accompanying notes!**
Currently, examples are available for the following platforms:

* [LoRaWAN for ESP32](https://github.com/radiolib-org/radiolib-persistence/tree/main/examples/LoRaWAN_ESP32)
* [LoRaWAN for ESP8266](https://github.com/radiolib-org/radiolib-persistence/tree/main/examples/LoRaWAN_ESP8266)

_This list is last updated at 30/03/2024._
