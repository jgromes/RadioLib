/*
   RadioLib HTTP POST Example

   This example sends HTTP POST request using ESP8266 WiFi module.

   Please note that the response will be saved including header. HTTP header size
   can easily exceed Arduino resources and cause the program to behave erratically.

   IMPORTANT: Before uploading this example, make sure that the ESP8266 module is running
   AT firmware (can be found in the /extras folder of the library)!

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
 */

// include the library
#include <RadioLib.h>

// ESP8266 has the following connections:
// TX pin: 9
// RX pin: 8
ESP8266 wifi = new Module(9, 8);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//ESP8266 wifi = RadioShield.ModuleA;

// create HTTP client instance using the wifi module
// the default port used for HTTP is 80
HTTPClient http(&wifi, 80);

void setup() {
  Serial.begin(9600);

  // initialize ESP8266
  Serial.print(F("[ESP8266] Initializing ... "));
  // baudrate:  9600 baud
  int state = wifi.begin(9600);
  if(state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
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
    Serial.print(F("failed, code "));
    Serial.println(state);
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
  if(http_code > 0) {
    Serial.print(F("HTTP code "));
    Serial.println(http_code);
    Serial.print(F("[ESP8266] Response is "));
    Serial.print(response.length());
    Serial.println(F(" bytes long."));
    Serial.println(response);
  } else {
    Serial.print(F("failed, code "));
    Serial.println(http_code);
  }

  // wait for a second before sending new request
  delay(1000);
}
