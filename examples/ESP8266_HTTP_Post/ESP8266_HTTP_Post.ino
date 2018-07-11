/*
 * KiteLib ESP8266 HTTP POST Example
 * 
 * This example sends HTTP POST request using ESP8266 WiFi module.
 * 
 * Please note that the response will be saved including header. HTTP header size
 * can easily exceed Arduino resources and cause the program to behave erratically.
 * 
 * IMPORTANT: Before uploading this example, make sure that the ESP8266 module is running
 * AT firmware (can be found in the /extras folder of the library)!
 */

// include the library
#include <KiteLib.h>

// ESP8266 module is in slot A on the shield
ESP8266 wifi = Kite.ModuleA;

// create HTTP client instance using the wifi module
HTTPClient http(&wifi);

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
  state = wifi.join("Tenda", "Student20-X13");
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(state, HEX);
    while(true);
  }

}

void loop() {
  // send HTTP POST request to www.httpbin.org/status/200
  // Content:         str
  // Content-Type:    text/plain
  String response;
  Serial.print(F("[ESP8266] Sending HTTP POST request ... "));
  int http_code = http.post("www.httpbin.org/status/404", "str", response);
  if(http_code >= 100) {
    Serial.print(F("HTTP code "));
    Serial.println(http_code);
    Serial.print(F("[ESP8266] Response is "));
    Serial.print(response.length());
    Serial.println(F(" bytes long."));
    Serial.println(response);
  } else {
    Serial.print(F("failed, code 0x"));
    Serial.println(http_code, HEX);
  }

  // wait for a second before sending new request
  delay(1000);
}

