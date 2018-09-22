#include <KiteLib.h>

RF69 rf = Kite.ModuleA;

void setup() {
  Serial.begin(9600);
  
  // initialize RF69 with default settings
  Serial.print(F("Initializing ... "));
  
  int state = rf.begin();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // set the function that will be called 
  // when new packet is received
  lora.setDio0Action(setFlag);
  
  // start listening for packets
  Serial.print(F("Starting to listen ... "));
  state = rf.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // lora.standby()
  // lora.sleep()
  // lora.transmit();
  // lora.receive();
  // lora.scanChannel();
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;
    
    // you can read received data as an Arduino String
    String str;
    int state = rf.readData(str);
  
    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = lora.receive(byteArr, 8);
    */
    
    if (state == ERR_NONE) {
      // packet was successfully received
      Serial.println("Received packet!");
  
      // print data of the packet
      Serial.print("Data:\t\t\t");
      Serial.println(str);
  
      // print RSSI (Received Signal Strength Indicator) 
      Serial.print("RSSI:\t\t\t");
      Serial.print(lora.lastPacketRSSI);
      Serial.println(" dBm");
    }

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }

}
