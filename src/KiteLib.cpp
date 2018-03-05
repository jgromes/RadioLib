#include "KiteLib.h"

/*Kite::Kite(ModuleType moduleA, ModuleType moduleB) {
  switch(moduleA) {
    case ModuleType::ESP8266:
      ESP8266 = new class ESP8266();
      break;
    case ModuleType::SX1278:
      SX1278 = new class SX1278();
      break;
  };
}*/

KiteShield::KiteShield() {
  ModuleA = new Module(KITE_CS_A, KITE_TX_A, KITE_RX_A, KITE_INT_0, KITE_INT_1);
  ModuleB = new Module(KITE_CS_B, KITE_TX_B, KITE_RX_B, KITE_INT_0, KITE_INT_1);
}

KiteShield Kite;
