/*
 * KiteLib ESP8266 MQTT Publish Example
 * 
 * This example publishes MQTT messages using ESP8266 WiFi module.
 * 
 * The messages are published to https://shiftr.io/try. You can use this namespace
 * for testing purposes, but remember that it is publicly accessible!
 * 
 * IMPORTANT: Before uploading this example, make sure that the ESP8266 module is running
 * AT firmware (can be found in the /extras folder of the library)!
 */

// include the library
#include <KiteLib.h>

// ESP8266 module is in slot A on the shield
ESP8266 wifi = Kite.ModuleA;

// create MQTT client instance using the wifi module
// the default port used for MQTT is 1883
MQTTClient mqtt(&wifi, 1883);

void setup() {
  Serial.begin(9600);

  // initialize ESP8266
  Serial.print(F("[ESP8266] Initializing ... "));
  // baudrate:  9600 baud
  byte state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

  // join access point
  Serial.print(F("[ESP8266] Joining AP ... "));
  // name:      SSID
  // password:  password
  state = wifi.join("SSID", "password");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

  // connect to MQTT server
  Serial.print(F("[ESP8266] Connecting to MQTT server ... "));
  // server URL:  broker.shiftr.io
  // client ID:   arduino
  // username:    try
  // password:    try
  state = mqtt.connect("broker.shiftr.io", "arduino", "try", "try");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  // publish MQTT message
  Serial.print(F("[ESP8266] Publishing MQTT message ... "));
  // topic name:            hello
  // application message:   world
  byte state = mqtt.publish("hello", "world");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
  }

  // wait for a second before publishing again
  delay(1000);
}

