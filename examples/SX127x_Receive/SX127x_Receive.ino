/*
 * KiteLib SX127x Receive Example
 * 
 * This example listens for LoRa transmissions using SX127x Lora modules.
 * To successfully receive data, the following settings have to be the same
 * on both transmitter and receiver:
 *  - carrier frequency
 *  - bandwidth
 *  - spreading factor
 *  - coding rate
 *  - sync word
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

  // you can receive data as an Arduino String
  String str;
  byte state = lora.receive(str);

  // you can also receive data as byte array
  /*
  byte byteArr[8];
  byte state = lora.receive(byteArr, 8);
  */

  if(state == ERR_NONE) {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print("[SX1278] Data:\t\t");
    Serial.println(str);

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
