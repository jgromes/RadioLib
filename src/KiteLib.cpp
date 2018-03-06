#include "KiteLib.h"

KiteShield::KiteShield() {
  ModuleA = new Module(KITE_CS_A, KITE_TX_A, KITE_RX_A, KITE_INT_0, KITE_INT_1);
  ModuleB = new Module(KITE_CS_B, KITE_TX_B, KITE_RX_B, KITE_INT_0, KITE_INT_1);
}

KiteShield Kite;
