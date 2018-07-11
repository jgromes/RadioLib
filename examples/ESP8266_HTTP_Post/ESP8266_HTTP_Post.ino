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
// the default port used for HTTP is 80
HTTPClient http(&wifi, 80);

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

}

void loop() {
  // send HTTP POST request to www.httpbin.org/status/404
  // the server doesn't process the posted data, it just returns
  // response with the status code 404
  String response;
  Serial.print(F("[ESP8266] Sending HTTP POST request ... "));
  // URL:             www.httpbin.org/status/404
  // content:         str
  // content type:    text/plain
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

