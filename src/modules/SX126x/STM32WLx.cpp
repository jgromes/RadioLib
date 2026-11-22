/*
Copyright (c) 2018 Jan Gromeš
Copyright (c) 2022 STMicroelectronics

This file is licensed under the MIT License: https://opensource.org/licenses/MIT
*/

#include "STM32WLx.h"
#if !RADIOLIB_EXCLUDE_STM32WLX

#if defined(ARDUINO_ARCH_STM32) && defined(STM32WLxx)
  #include <SubGhz.h>
#endif

#if defined(STM32CubeWL)
  #include "stm32wlxx_hal.h"
#endif

STM32WLx::STM32WLx(Module* mod) : SX1262(mod) {
  this->powerMin = RADIOLIB_STM32WLX_LP_POUT_MIN;
  this->powerMax = RADIOLIB_STM32WLX_HP_POUT_MAX;
  this->paSteps = this->powerMax - this->powerMin + 1;
}

int16_t STM32WLx::begin(const ConfigLoRa_t& cfg) {
  // Execute common part
  int16_t state = SX1262::begin(cfg);
  RADIOLIB_ASSERT(state);

  // This overrides the value in SX126x::begin()
  // On STM32WL, DIO2 is hardwired to the radio IRQ on the MCU, so it
  // should really not be used as RfSwitch control output.
  state = setDio2AsRfSwitch(false);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t STM32WLx::beginFSK(const ConfigFSK_t& cfg) {
  // Execute common part
  int16_t state = SX1262::beginFSK(cfg);
  RADIOLIB_ASSERT(state);

  // This overrides the value in SX126x::begin()
  // On STM32WL, DIO2 is hardwired to the radio IRQ on the MCU, so it
  // should really not be used as RfSwitch control output.
  state = setDio2AsRfSwitch(false);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t STM32WLx::setOutputPower(int8_t power) {
  // apply offset for external PA
  int8_t pwr = power;
  int16_t state = this->applyOutputPowerOffset(RADIOLIB_STM32WLX_LP_POUT_MIN, &power, &pwr);
  RADIOLIB_ASSERT(state);

  // get current OCP configuration
  uint8_t ocp = 0;
  state = readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  RADIOLIB_ASSERT(state);

  // check the user did not request power output that is not possible
  const Module* mod = this->getMod();
  bool hp_supported = mod->findRfSwitchMode(MODE_TX_HP);
  bool lp_supported = mod->findRfSwitchMode(MODE_TX_LP);

  // check whether any RF switch configuration exists at all - if standby does not exist, then consider the module switchless
  if(!mod->findRfSwitchMode(MODE_IDLE)) {
    lp_supported = true;
    hp_supported = true;
  }

  if((!lp_supported && (pwr < RADIOLIB_STM32WLX_HP_POUT_MIN)) || (!hp_supported && (pwr > RADIOLIB_STM32WLX_LP_POUT_MAX))) {
    // LP not supported but requested power is below HP low bound or
    // HP not supported but requested power is above LP high bound
    return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }
  
  // determine which PA to use (the default is for LP)
  bool use_hp = false;
  this->txMode = MODE_TX_LP;

  // if HP is supported, use it in the following cases:
  // 1. HP is the only option or
  // 2. power requested exceeds the LP range
  if(hp_supported && ((!lp_supported) || (pwr > RADIOLIB_STM32WLX_LP_POUT_MAX))) {
    use_hp = true;
    this->txMode = MODE_TX_HP;
  }

  state = SX126x::setPaConfig(0x04, !use_hp, use_hp ? 0x07 : 0x00);
  RADIOLIB_ASSERT(state);

  // apply workaround for HP only
  state = SX126x::fixPaClamping(use_hp);
  RADIOLIB_ASSERT(state);

  // set output power with default 200us ramp
  state = SX126x::setTxParams(pwr, RADIOLIB_SX126X_PA_RAMP_200U);
  RADIOLIB_ASSERT(state);

  // restore OCP configuration
  return(writeRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
}

int16_t STM32WLx::clearIrqStatus(uint16_t clearIrqParams) {
  int16_t res = SX126x::clearIrqStatus(clearIrqParams);
  // The NVIC interrupt is level-sensitive, so clear away any pending
  // flag that is only set because the radio IRQ status was not cleared
  // in the interrupt (to prevent each IRQ triggering twice and allow
  // reading the irq status through the pending flag).
#if defined(ARDUINO_ARCH_STM32)
  SubGhz.clearPendingInterrupt();
  if(SubGhz.hasInterrupt())
    SubGhz.enableInterrupt();
#elif defined(STM32CubeWL)
  HAL_NVIC_ClearPendingIRQ(SUBGHZ_Radio_IRQn);
  HAL_NVIC_EnableIRQ(SUBGHZ_Radio_IRQn);
#endif
  return(res);
}

#if defined(ARDUINO_ARCH_STM32)
void STM32WLx::setIrqAction(void (*func)(void), bool rising) {
  (void)rising;
  SubGhz.attachInterrupt([func]() {
    // Because the interrupt is level-triggered, we disable it in the
    // NVIC (otherwise we would need an SPI command to clear the IRQ in
    // the radio, or it would trigger over and over again).
    SubGhz.disableInterrupt();
    func();
  });
}

void STM32WLx::clearIrqAction() {
  SubGhz.detachInterrupt();
}
#endif  // ARDUINO_ARCH_STM32

#endif
