/*
 * KiteLib ESP8266 MQTT Publish Example
 * 
 * This example publishes MQTT messages using ESP8266 WiFi module.
 * 
 * The messages are published to https://shiftr.io/try. You can use this namespace
 * for testing purposes, but remember that it is publicly accessible!
 * 
 * IMPORTANT: Before upolading this example, make sure that the ESP8266 module is running
 * AT firmware (can be found in the /extras folder of the library)!
 */

// include the library
#include <KiteLib.h>

// ESP8266 module is in slot A on the shield
ESP8266 wifi = Kite.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize ESP8266 with baudrate 9600
  Serial.print(F("[ESP8266] Initializing ... "));
  byte state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

  // join AP named "SSID" using the password "password"
  Serial.print(F("[ESP8266] Joining AP ... "));
  state = wifi.join("SSID", "password");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

  // connect to MQTT broker using client ID "arduino", username "try" and password "try"
  Serial.print(F("[ESP8266] Connecting to MQTT broker ... "));
  state = wifi.MqttConnect("broker.shiftr.io", "arduino", "try", "try");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }
}

void loop() {
  // publish MQTT message to the topic "hello" with content "world"
  Serial.print(F("[ESP8266] Publishing MQTT message ... "));
  byte state = wifi.MqttPublish("hello", "world");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
  }

  // wait for a second before publishing again
  delay(1000);
}

