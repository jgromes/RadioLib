/*
   RadioLib SX127x Transmit with Frequency Hopping Example

   This example transmits packets using SX1278 LoRa radio module.
   Each packet contains up to 256 bytes of data, in the form of:
    - Arduino String
    - null-terminated char array (C-string)
    - arbitrary binary data (byte array)

   Other modules from SX127x/RFM9x family can also be used.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/

   The SX1276 / 7 / 8 / 9 supports FHSS or Frequency Hopping Spread Spectrum.
   Once a hopping period is set and a transmission is started the radio
   will begin triggering interrupts every hop period where the radio frequency
   is changed to the next channel.
*/

#include <RadioLib.h> //Click here to get the library: http://librarymanager/All#RadioLib

// SX1276 has the following connections:
const int pin_cs = 10;
const int pin_dio0 = 2;
const int pin_dio1 = 9;
const int pin_rst = 3;
SX1276 radio = new Module(pin_cs, pin_dio0, pin_rst, pin_dio1);

volatile bool xmitComplete = false;
volatile bool fhssChange = false;

// the channel frequencies can be generated randomly or hard coded
float channels[] = {908.0, 906.0, 907.0, 905.0, 903.0, 910.0, 909.0};
int numberOfChannels = sizeof(channels) / sizeof(float);

int hopsCompleted = 0;
int counter = 0;

void setup() {
  Serial.begin(9600);

  // begin radio on home channel
  Serial.print(F("[SX127x] Initializing ... "));
  int state = radio.begin(channels[0]);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Failed with code: "));
    Serial.println(state);
  }
  else
    Serial.println(F("Success!"));

  // set hop period to enable FHSS
  state = radio.setFHSSHoppingPeriod(9);
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Error setting hopping period: "));
    Serial.println(state);
  }

  radio.setDio0Action(dio0ISR); // called when transmission is finished
  radio.setDio1Action(dio1ISR); // called after a transmission has started, so we can move to next freq

  Serial.print(F("Transmitting packet..."));

  String longOutput = "Let's create a really long packet to trigger lots of hop interrupts. A packet can be up to 256 bytes long. This packet is 222 bytes so using sf = 9, bw = 125, timeOnAir is 1488ms. 1488ms / (9*4.10ms) = 40 hops. Counter: ";

  state = radio.startTransmit(longOutput + counter);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Error transmitting with code: "));
    Serial.println(state);
  }
}

void loop() {
  if (xmitComplete == true) {
    xmitComplete = false;
    Serial.println(F("Transmit complete"));
    Serial.print(F("Radio after xmit is on channel: "));
    Serial.println(radio.getFHSSChannel());
    // the FHSS channel is automatically reset to 0 upon end of transmission

    radio.setFrequency(channels[radio.getFHSSChannel() % numberOfChannels]); // Return to home channel before next transaction

    Serial.print(F("Hops completed: "));
    Serial.println(hopsCompleted);
    hopsCompleted = 0;

    radio.startReceive();
  }

  if (fhssChange == true) {
    radio.setFrequency(channels[radio.getFHSSChannel() % numberOfChannels]);

    hopsCompleted++;
    fhssChange = false;
    radio.clearFHSSInt();
  }
}

// ISR when DIO0 goes low
// called when transmission is complete or when RX is received
void dio0ISR(void) {
  xmitComplete = true;
}

// ISR when DIO1 goes low
// called when FhssChangeChannel interrupt occurs (at regular HoppingPeriods)
void dio1ISR(void) {
  fhssChange = true;
}
