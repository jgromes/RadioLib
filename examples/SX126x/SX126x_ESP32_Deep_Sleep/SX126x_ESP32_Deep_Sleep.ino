#if !defined(ESP32)
  #error This example is only for ESP32-based boards!
#endif
/*
  RadioLib SX126x ESP32 Deep Sleep Example

  This example listens and receives LoRa transmissions while
  deep sleeping the ESP32 MCU. Once a packet is received, an
  interrupt is triggered that awakens the ESP32. To successfully
  receive data, the following settings have to be the same on
  both transmitter and receiver:
  - carrier frequency
  - bandwidth
  - spreading factor
  - coding rate
  - sync word

  Other modules from SX126x family can also be used.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>
#include <driver/rtc_io.h>

// SX1262 has the following connections:
#define SX1262_NSS 10
#define SX1262_DIO1 2 // MUST be a RTC GPIO to support deep sleep
#define SX1262_NRST 3
#define SX1262_BUSY 9
SX1262 radio = new Module(SX1262_NSS, SX1262_DIO1, SX1262_NRST, SX1262_BUSY);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// timer to prevent sleep
#define SLEEP_AFTER_MS 5000
unsigned long sleepAfter = 0;

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
ICACHE_RAM_ATTR void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void setup() {
  Serial.begin(9600);
  // give some time for the serial to connect
  delay(2000);
  sleepAfter = millis() + SLEEP_AFTER_MS; // Deep sleep ESP32 after 5 seconds of inactivity

  if (!rtc_gpio_is_valid_gpio((gpio_num_t)SX1262_DIO1)) {
    Serial.println(F("[ESP32] Deep sleep requires DIO1 be wired to a RTC GPIO"));
    while (true) { delay(10); }
  }

  auto wakeupReason = esp_sleep_get_wakeup_cause();
  if (wakeupReason == ESP_SLEEP_WAKEUP_EXT1) {
    Serial.println(F("[ESP32] Woke from deep sleep by radio interrupt"));
  } else if (wakeupReason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println(F("[ESP32] Woke from deep sleep by timer"));
  } else if (wakeupReason == ESP_SLEEP_WAKEUP_UNDEFINED) {
    Serial.println(F("[ESP32] Power-on or hard reset"));
  } else {
    Serial.print(F("[ESP32] Woke from deep sleep by other reason: "));
    Serial.println(wakeupReason);
  }

  bool resetModule = true;
  if (wakeupReason != ESP_SLEEP_WAKEUP_UNDEFINED) {
    // Only reset the module after a power-on/hard reset
    resetModule = false;
    // For any reason except a power-on/hard reset, check if DIO1 is high and set
    // the interrupt flag. Even if we woke due to a timer or another pin, DIO1
    // may be high if a packet arrived shortly after wakeup was triggered.
    pinMode(SX1262_DIO1, INPUT_PULLDOWN);
    if (digitalRead(SX1262_DIO1) == HIGH) {
      Serial.println(F("[ESP32] DIO1 is high, setting received flag"));
      setFlag();
    }
  }

  // initialize SX1262
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin(434.0, 125.0, 9, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 10, 8, 1.6, false, resetModule);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;
    // reset sleep timer
    sleepAfter = millis() + SLEEP_AFTER_MS;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int numBytes = radio.getPacketLength();
      int state = radio.readData(byteArr, numBytes);
    */

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1262] Received packet!"));

      // print data of the packet
      Serial.print(F("[SX1262] Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[SX1262] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[SX1262] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

      // print frequency error
      Serial.print(F("[SX1262] Frequency error:\t"));
      Serial.print(radio.getFrequencyError());
      Serial.println(F(" Hz"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("[SX1262] CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("[SX1262] readData failed, code "));
      Serial.println(state);

    }
  }

  if ((long)(millis() - sleepAfter) > 0) {
    Serial.println(F("[ESP32] Going to sleep now!"));

    radio.clearPacketReceivedAction(); // Disable interrupt on DIO1 pin

    uint64_t io_mask = 0;
    io_mask |= (1ULL << SX1262_DIO1);
    esp_err_t result = esp_sleep_enable_ext1_wakeup(io_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    if (result != ESP_OK) {
        Serial.print(F("[ESP32] Failed to configure ext1 wakeup, error code: "));
        Serial.println(result);
        sleepAfter = millis() + 1000; // don't try again immediately
        return;
    }

    Serial.flush();

    // put ESP32 into deep sleep - this will not return
    esp_deep_sleep_start();
    // not reached
  }
}
