# LoRaWAN examples
RadioLib LoRaWAN examples.

* [LoRaWAN_Starter](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Starter): this is the recommended entry point for new users. Please read the [`notes`](https://github.com/jgromes/RadioLib/blob/master/examples/LoRaWAN/LoRaWAN_Starter/notes.md) that come with this example to learn more about LoRaWAN and how to use it in RadioLib!
* [LoRaWAN_Reference](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_Reference): this sketch showcases most of the available API for LoRaWAN in RadioLib. Be frightened by the possibilities! It is recommended you have read all the [`notes`](https://github.com/jgromes/RadioLib/blob/master/examples/LoRaWAN/LoRaWAN_Starter/notes.md) for the Starter sketch first, as well as the [Learn section on The Things Network](https://www.thethingsnetwork.org/docs/lorawan/)!
* [LoRaWAN_ABP](https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN/LoRaWAN_ABP): if you wish to use ABP instead of OTAA (but why?), this example shows how you can do this using RadioLib.

## LoRaWAN versions & regional parameters
RadioLib implements both LoRaWAN Specification 1.1 and 1.0.4. Confusingly, 1.0.4 is newer than 1.1, but 1.1 includes more security checks and as such **LoRaWAN 1.1 is preferred**.  
The catch is in the Regional Parameters: as RP002 1.0.4 is newer than RP001 1.1, it is more up to date regarding local laws & regulations. Therefore, RadioLib implements 1.0.4 as baseline and 1.1 (revision B) as fallback, and as such **RP002 Regional Parameters 1.0.4 is preferred**.
_Note: the CN500 band is implemented as specified in RP001 1.1 revision B, as the RP002 1.0.4 version is much too complex._

To activate a LoRaWAN 1.1 session, supply all the required keys:
```cpp
node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
node.beginABP(devAddr, fNwkSIntKey, sNwkSIntKey, nwkSEncKey, appSKey);
```

To activate a LoRaWAN 1.0.4 session, set the keys that are not available to `NULL`:
```cpp
node.beginOTAA(joinEUI, devEUI, NULL, appKey);
node.beginABP(devAddr, NULL, NULL, nwkSEncKey, appSKey);
```

The device doesn't need to know the Regional Parameters version - that is of importance on the console.

## LoRaWAN persistence
> [!WARNING]
> These examples do not actually comply with LoRaWAN 1.0.4/1.1: for that, persistent storage is necessary. As the implementation of persistent storage differs between different platforms, these are not given here, but in a separate repository, see below:

In [this repository](https://github.com/radiolib-org/radiolib-persistence), examples are provided that do comply with the required persistence of certain parameters for LoRaWAN 1.1. Examples are (or will become) available for some of the most popular platforms. **These examples assume you have successfully used the Starter sketch and understood (most of) the accompanying notes!**
Currently, examples are available for the following platforms:

* [LoRaWAN for ESP32](https://github.com/radiolib-org/radiolib-persistence/tree/main/examples/LoRaWAN_ESP32)
* [LoRaWAN for ESP8266](https://github.com/radiolib-org/radiolib-persistence/tree/main/examples/LoRaWAN_ESP8266)

_This list is last updated at 30/03/2024._
