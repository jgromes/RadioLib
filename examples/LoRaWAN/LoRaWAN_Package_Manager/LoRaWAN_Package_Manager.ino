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
LoRaWANPackageManager pacMan(&node, getSeconds);

// Forward declaration of downlink processing function
void processDownlink(int16_t state);

void setup() {
  Serial.begin(115200);
  while(!Serial);

  // Initialize radio and LoRaWAN node (omitted error handling)
  radio.begin({});
  node.beginOTAA(joinEUI, devEUI, NULL, appKey);  // LoRaWAN v1.0.4 - no NwkKey

  // Warning: radio.begin() must be called before enabling packages!

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

  // Check if the package manager requests confirmed uplinks
  confirmed = pacMan.getConfirmed();

  // Get current time
  RadioLibTime_t now = getSeconds();

  // Check package manager for next task
  LoRaWANTaskInfo task = pacMan.hasTask();

  // Perform an action if required
  if(task.type == RADIOLIB_LORAWAN_TASK_ACTION && task.time <= now) {
    pacMan.doAction();
    return;
  }

  // Send an uplink if required
  if(task.type == RADIOLIB_LORAWAN_TASK_UPLINK && task.time <= now) {
    // Can only send once dutycycle allows
    if(node.timeUntilUplink() > 0) {
      return;
    }

    // Get uplink data from package manager and send it
    if(pacMan.getUplinkData(uplink, &uplLen, &fPort)) {
      lastUplinkTime = now;
      int16_t state = node.sendReceive(uplink, uplLen, fPort, downlink, &downLen, confirmed, &evtUp, &evtDown);
      processDownlink(state);

    }
    return;
  }


  // Send normal application uplinks at regular intervals
  // This would be where you read your sensors and send regular updates
  if(now - lastUplinkTime >= uplinkIntervalSeconds && node.timeUntilUplink() == 0) {
    lastUplinkTime = now;

    // Dummy data
    uint8_t payload[2] = { 0xAA, 0x55 };
    int16_t state = node.sendReceive(payload, sizeof(payload), 1, downlink, &downLen, confirmed, &evtUp, &evtDown);
    processDownlink(state);
  }

}

// Function to process downlink data and forward package downlinks to package manager
void processDownlink(int16_t state) {
  if(state < RADIOLIB_ERR_NONE) {
    Serial.print(F("Error during sendReceive: "));
    Serial.println(state);
    return;
  }
  if(state == RADIOLIB_ERR_NONE || downLen == 0) {
    Serial.println(F("No downlink data"));
    return;
  }

  Serial.println(F("Received downlink data:"));

  // Forward package downlinks to package manager
  if(downLen > 0 && pacMan.isEnabledFPort(evtDown.fPort)) {
    Serial.println(F("It is a package downlink"));
    pacMan.processDownlink(downlink, downLen, &evtDown);

  // Process normal downlinks for ourselves
  } else if(downLen > 0) {
    arrayDump(downlink, downLen);
  }
}