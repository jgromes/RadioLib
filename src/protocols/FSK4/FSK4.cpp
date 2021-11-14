#include "FSK4.h"
#if !defined(RADIOLIB_EXCLUDE_FSK4)

FSK4Client::FSK4Client(PhysicalLayer* phy) {
  _phy = phy;
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  _audio = nullptr;
  #endif
}

#if !defined(RADIOLIB_EXCLUDE_AFSK)
 FSK4Client::FSK4Client(AFSKClient* audio) {
   _phy = audio->_phy;
   _audio = audio;
 }
#endif

int16_t FSK4Client::begin(float base, uint32_t shift, uint16_t rate) {
  // save configuration
  _baseHz = base;
  _shiftHz = shift;

  // calculate duration of 1 bit
  _bitDuration = (uint32_t)1000000/rate;

  // calculate module carrier frequency resolution
  uint32_t step = round(_phy->getFreqStep());

  // check minimum shift value
  if(shift < step / 2) {
    return(RADIOLIB_ERR_INVALID_RTTY_SHIFT);
  }

  // round shift to multiples of frequency step size
  if(shift % step < (step / 2)) {
    _shift = shift / step;
  } else {
    _shift = (shift / step) + 1;
  }

  // Write resultant tones into arrays for quick lookup when modulating.
  _tones[0] = 0;
  _tones[1] = _shift;
  _tones[2] = _shift*2;
  _tones[3] = _shift*3;

  _tonesHz[0] = 0;
  _tonesHz[1] = _shiftHz;
  _tonesHz[2] = _shiftHz*2;
  _tonesHz[3] = _shiftHz*3;

  // calculate 24-bit frequency
  _base = (base * 1000000.0) / _phy->getFreqStep();

  // configure for direct mode
  return(_phy->startDirect());
}

void FSK4Client::idle() {
  // Idle at Tone 0.
  tone(0);
}

size_t FSK4Client::write(uint8_t* buff, size_t len) {
  size_t n = 0;
  for(size_t i = 0; i < len; i++) {
    n += FSK4Client::write(buff[i]);
  }
  FSK4Client::standby();
  return(n);
}

size_t FSK4Client::write(uint8_t b) {
  // send symbols MSB first
  for(uint8_t i = 0; i < 4; i++) {
    // Extract 4FSK symbol (2 bits)
    uint8_t symbol = (b & 0xC0) >> 6;

    // Modulate
    FSK4Client::tone(symbol);

    // Shift to next symbol
    b = b << 2;
  }

  return(1);
}

void FSK4Client::tone(uint8_t i) {
  Module* mod = _phy->getMod();
  uint32_t start = mod->micros();
  transmitDirect(_base + _tones[i], _baseHz + _tonesHz[i]);
  while(mod->micros() - start < _bitDuration) {
    mod->yield();
  }
}

int16_t FSK4Client::transmitDirect(uint32_t freq, uint32_t freqHz) {
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio != nullptr) {
    return(_audio->tone(freqHz));
  }
  #endif
  return(_phy->transmitDirect(freq));
}

int16_t FSK4Client::standby() {
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio != nullptr) {
    return(_audio->noTone());
  }
  #endif
  return(_phy->standby());
}

#endif
