/*
  RadioLib LoRaWAN Package Manager Example

  This example shows basic usage of `LoRaWANPackageManager`:
  - Create package manager
  - Query `hasTask()` for next package task (UPLINK or ACTION)
  - If task is UPLINK: call `getUplinkData()` and send
  - If task is ACTION: call `doAction()`
  - Forward package downlinks to `processDownlink()`

  Do NOT run this example before trying out the Starter and Reference examples.
  Do NOT run this example without reading through it and understanding how it works. 
  The package manager is a powerful tool but requires a good understanding 
  of the LoRaWAN protocol and the package manager architecture to use effectively.

  This example is difficult at first sight. However, adding new
  packages down the line will not require any modification to this
  example apart from enabling the packages in setup().

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN
*/

#include "config.h"
#include "example.h"

// Create the package manager and give it a notion of current time in seconds
LoRaWANPackageManager pacMan(&hal, &node, getSeconds);

// Forward declaration of uplink/downlink function
void sendReceive();

void setup() {
  Serial.begin(115200);
  while(!Serial);

  // Initialize radio and LoRaWAN node (omitted error handling)
  ConfigLoRa_t config;
  config.frequency = 868; // The frequency here does not matter, as it will get changed by LoRaWAN anyway
  //radio.tcxoVoltage = 1.6; // Some radio modules like SX126x often come with TCXO
  radio.begin(config);
  node.beginOTAA(joinEUI, devEUI, NULL, appKey);  // LoRaWAN v1.0.4 - no NwkKey

  // Warning: radio.begin() and loadBuffers() must be called before enabling packages!

  // Enable TS003 (Application Time) on default FPort
  pacMan.enableTS003(RADIOLIB_LORAWAN_FPORT_TS003, setSeconds);
  // Enable TS009 (Certification Protocol) with delay, interval, and reboot callbacks
  pacMan.enableTS009(&radio, delaySeconds, setUplinkInterval, performReboot);

  // Activate a.k.a. join the network
  node.activateOTAA();
}

uint8_t uplink[RADIOLIB_LORAWAN_MAX_PAYLOAD_SIZE], downlink[RADIOLIB_LORAWAN_MAX_PAYLOAD_SIZE];
size_t uplLen = 0, downLen = 0;
uint8_t fPort;
bool confirmed = false;
uint32_t lastUplinkTime = 0;
LoRaWANEvent_t evtUp, evtDown;

void loop() {
  // Ensure the node is activated before proceeding
  if(!node.isActivated()) {
    node.activateOTAA();

    if(!node.isActivated()) {
      delay(uplinkIntervalSeconds * 1000);
      return;
    }
  }

  // Get current time
  tNow = getSeconds();

  // Let the package manager execute any due tasks and report when it next needs servicing
  bool update = false;
  if(pacMan.handleTask(&tNextTask) && tNextTask <= tNow) {
    pacMan.getUplinkData(uplink, &uplLen, &fPort);
    sendReceive();
    update = true;
  }

  // If there is no package uplink, send normal user uplinks at regular intervals
  // This would be where you read your sensors
  else if(tNextUplink <= tNow) {
    uplink[0] = 0xAA;
    uplink[1] = 0x55;
    uplLen = 2;
    fPort = 1;
    sendReceive();
    update = true;
  }

  // If we did something, update the time of next uplink and next task
  if(update) {
    // Next uplink: at scheduled interval, unless constrained by dutycycle
    tNextUplink = getSeconds() + RADIOLIB_MAX(uplinkIntervalSeconds - node.getLastDuration(true),
                                              node.timeUntilUplink(true));

    // Update the timestamp of the next task as we may have processed something
    pacMan.handleTask(&tNextTask);
  }

  // If Class A, we can delay/sleep until the next event
  if(node.getClass() == RADIOLIB_LORAWAN_CLASS_A) {
    // Check which is first: package task or regular uplink
    uint32_t tNext = RADIOLIB_MIN(tNextTask, tNextUplink);

    // If next action is in the future, await it
    tNow = getSeconds();
    delay((tNext - tNow) * 1000);

    continue;
  }

  // Otherwise (Class C), check if there is a downlink ready for processing
  if(node.getDownlinkClassC(downlink, &downLen, &evtDown) > 0) {
    Serial.println(F("Received a Class C downlink"));

    if(downLen > 0) {
      pacMan.processDownlink(downlink, downLen, &evtDown);
    }

    // Print extra information about the event
    Serial.print(F("\tFCnt: "));
    Serial.println(evtDown.fCnt);
    Serial.print(F("\tPort: "));
    Serial.println(evtDown.fPort);
    Serial.print(F("\tCast: "));
    Serial.println(evtDown.multicast ? "Multi" : "Uni");
    continue;
  }
}

// Function to process downlink data and forward package downlinks to package manager
void sendReceive() {
  // send uplink and listen for downlinks
  int16_t state = node.sendReceive(uplink, uplLen, fPort, downlink, &downLen, confirmed, &evtUp, &evtDown);

  // Check for an error
  if(state < RADIOLIB_ERR_NONE) {
    Serial.print(F("Error during sendReceive: "));
    Serial.println(state);
    return;
  }

  // Check for a downlink
  if(state == RADIOLIB_ERR_NONE) {
    return;
  }

  // If we got a downlink, check if it carries DeviceTimeAns
  uint32_t timestamp = 0;
  uint16_t fraction = 0;
  if(node.getMacDeviceTimeAns(&timestamp, &fraction, false) == RADIOLIB_ERR_NONE) {
    setSeconds(timestamp);
    Serial.println(F("Set time using DeviceTime"));
  }

  // Did it contain any application data?
  if(downLen == 0) {
    Serial.println(F("MAC-only downlink"));
    return;
  }

  // Forward package downlinks to package manager
  if(pacMan.isEnabledFPort(evtDown.fPort)) {
    Serial.println(F("Received a package downlink"));
    pacMan.processDownlink(downlink, downLen, &evtDown);
    return;
  }

  // Finally, if we get here, process a normal downlinks for ourselves
  Serial.println(F("Downlink with user-data:"));
  for(size_t i = 0; i < downLen; i++) {
    Serial.print(downlink[i], HEX);
  }
  Serial.println();
}
