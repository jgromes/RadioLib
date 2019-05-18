#ifndef _RADIOLIB_SX1261_H
#define _RADIOLIB_SX1261_H

#include "TypeDef.h"
#include "Module.h"
#include "SX126x.h"
#include "SX1262.h"

//SX126X_CMD_SET_PA_CONFIG
#define SX126X_PA_CONFIG_SX1261                       0x01
#define SX126X_PA_CONFIG_SX1262                       0x00

// TODO: implement SX1261 class
using SX1261 = SX1262;

#endif
