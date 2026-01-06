/*
   RadioLib NiceRF LoRa1121F33-2G4 Ping-Pong Example

   For the module description, see the manufacturer page
   https://www.nicerf.com/lora-module/lora-1121f33-2g4.html

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr11x0---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// uncomment the following only on one
// of the nodes to initiate the pings
//#define INITIATING_NODE

// #define RADIO_CE           5UL

// LR1110 has the following connections:
// NSS pin:   7
// IRQ pin:   15
// NRST pin:  4
// BUSY pin:  44
// CE pin:    5
LR1120 radio = new Module(7, 15, 4, 44);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// set RF switch configuration for LoRa1121F33-2G4
// It uses DIO5, DIO6, DIO7 and DIO8 for RF switching
// NOTE: LoRa1121F33-1G9 is different!
static const uint32_t rfswitch_dio_pins[] = { 
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6, RADIOLIB_LR11X0_DIO7, 
    RADIOLIB_LR11X0_DIO8, RADIOLIB_NC
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    // mode                  DIO5  DIO6  DIO7  DIO8
    { LR11x0::MODE_STBY,   { LOW,  LOW,  LOW,  LOW  } },
    { LR11x0::MODE_RX,     { LOW,  LOW,  LOW,  LOW  } },
    { LR11x0::MODE_TX,     { LOW,  LOW,  LOW,  LOW  } },
    { LR11x0::MODE_TX_HP,  { LOW,  LOW,  LOW,  HIGH } }, //subghz DIO8
    { LR11x0::MODE_TX_HF,  { LOW,  HIGH, HIGH, LOW  } }, //2G4 DIO6
    { LR11x0::MODE_GNSS,   { LOW,  LOW,  LOW,  LOW  } },
    { LR11x0::MODE_WIFI,   { HIGH, LOW,  LOW,  LOW  } }, //2G4_RXEN DIO5
    END_OF_MODE_TABLE,
};

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent or received a packet, set the flag
  operationDone = true;
}

void setup() {
  //SPI.begin(RADIO_SCK, RADIO_MISO, RADIO_MOSI, RADIO_NSS);

  // Has internal pullup
  // pinMode(RADIO_CE, OUTPUT);
  // digitalWrite(RADIO_CE, HIGH);

  Serial.begin(115200);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  radio.XTAL = false;
  int state = radio.begin(434.0, 125.0, 9, 7, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, 1, 8, 3.0);
  //int state = radio.begin(2450.0, 812.50, 7, 5, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, 1, 8, 3.0);
  //int state = radio.begin(2450.0, 7.8, 7, 5, RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE, 1, 8, 3.0);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set RF switch control configuration
  radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

  // set the function that will be called
  // when new packet is received
  radio.setIrqAction(setFlag);

  radio.setOutputPower(1, true);
  radio.setRxBoostedGainMode(true);

  #if defined(INITIATING_NODE)
    // send the first packet on this node
    Serial.print(F("[LR1110] Sending first packet ... "));
    transmissionState = radio.startTransmit("Hello World!");
    transmitFlag = true;
  #else
    // start listening for LoRa packets on this node
    Serial.print(F("[LR1110] Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while (true) { delay(10); }
    }
  #endif
}

void loop() {
  // check if the previous operation finished
  if(operationDone) {
    // reset flag
    operationDone = false;

    if(transmitFlag) {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {
      // the previous operation was reception
      // print data and send another packet
      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        Serial.println(F("[LR1110] Received packet!"));

        // print data of the packet
        Serial.print(F("[LR1110] Data:\t\t"));
        Serial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[LR1110] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[LR1110] SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));

      }

      // wait a second before transmitting again
      delay(1000);

      // send another one
      Serial.print(F("[LR1110] Sending another packet ... "));
      transmissionState = radio.startTransmit("Hello World!");
      transmitFlag = true;
    }
  
  }
}
