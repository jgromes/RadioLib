/*
 * KiteLib SX127x Receive Example
 * 
 * This example receives packets using SX1278 LoRa radio module.
 * 
 * Other modules from SX127x family can also be used.
 * SX1272 lora = Kite.ModuleA;
 * SX1273 lora = Kite.ModuleA;
 * SX1276 lora = Kite.ModuleA;
 * SX1277 lora = Kite.ModuleA;
 * SX1279 lora = Kite.ModuleA;
 */

// include the library
#include <KiteLib.h>

// SX1278 module is in slot A on the shield
SX1278 lora = Kite.ModuleA;

// create empty instance of Packet class
Packet pack;

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  // carrier frequency:                   434.0 MHz
  // bandwidth:                           125.0 kHz
  // spreading factor:                    9
  // coding rate:                         7
  // sync word:                           0x12
  // output power:                        17 dBm
  // node address in EEPROM starts at:    0
  byte state = lora.begin();
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  Serial.print(F("[SX1278] Waiting for incoming transmission ... "));

  // wait for single packet
  byte state = lora.receive(pack);

  if(state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the source of the packet
    Serial.print(F("[SX1278] Source:\t\t"));
    Serial.println(pack.getSourceStr());

    // print the destination of the packet
    Serial.print(F("[SX1278] Destination:\t"));
    Serial.println(pack.getDestinationStr());

    // print the length of the packet
    Serial.print(F("[SX1278] Length:\t\t"));
    Serial.println(pack.length);

    // print the data of the packet
    Serial.print(F("[SX1278] Data:\t\t"));
    Serial.println(pack.data);

    //print the measured data rate
    Serial.print("[SX1278] Datarate:\t");
    Serial.print(lora.dataRate);
    Serial.println(" bps");

    //print the RSSI (Received Signal Strength Indicator) of the last received packet
    Serial.print("[SX1278] RSSI:\t\t");
    Serial.print(lora.lastPacketRSSI);
    Serial.println(" dBm");

    //print the SNR (Signal-to-Noise Ratio) of the last received packet
    Serial.print("[SX1278] SNR:\t\t");
    Serial.print(lora.lastPacketSNR);
    Serial.println(" dBm");
    
  } else if(state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));
    
  } else if(state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));
    
  }
}
