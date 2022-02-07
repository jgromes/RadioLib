/*
   RadioLib SX127x FSK FIFO on-the-fly Refilling Example

   This example shows how to use refill the FIFO buffer in SX127x chips, to allow for transmitting
   FSK/OOK packets longer than 256 bytes.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---fsk-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1276 has the following connections:
// NSS pin:   8
// DIO0 pin:  51
// RESET pin: 22
// DIO1 pin:  52
SX1276 radio = new Module(8, 51, 22, 18);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1278 fsk = RadioShield.ModuleA;

uint8_t syncWord[] = { 0x91, 0xD3, 0x91, 0xD3 };

char* msg = "This is just some message that extends well beyond the 64 bytes limit of the FIFO";
size_t msg_length;
size_t msg_counter;

// this interrupt is called whenever the FIFO buffer falls below a certain threshold
void fifoRefill() {
  radio.fifoAppend(
    (uint8_t*)msg,    // start of message, offset by # of bytes already sent
    msg_length,       // # of bytes left to send
    msg_counter
  );
  Serial.print("ISR ");
  Serial.print(msg_counter);
  Serial.println();
}

void setup() {
  Serial.begin(9600);

  // RF switch for my board
  pinMode(47, OUTPUT);
  digitalWrite(47, LOW);

  // Set interrupt as input
  pinMode(18, INPUT);

  // initialize SX1278 FSK modem with default settings
  Serial.print(F("[SX1276] Initializing ... "));
  int state = radio.beginFSK();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // Set config options
  radio.setDataShaping(RADIOLIB_SHAPING_NONEg);
  radio.setSyncWord(syncWord, sizeof(syncWord));
  radio.setEncoding(RADIOLIB_ENCODING_WHITENING);
  radio.setCRC(true);

  // Set up FIFO operations
  radio.setFifoThreshold(16);
  radio.setFifoThresholdAction(fifoRefill);
}

void loop() {

  radio.variablePacketLengthMode();

  // set RF switch to TX mode
  digitalWrite(47, LOW);

  // start transmission of message
  msg_length = strlen(msg);
  Serial.println("[SX1276] Starting transmission...\n\t");
  int stat = radio.startTransmit((uint8_t*)msg, msg_length, msg_counter, 0);
  delay(2000);
}
