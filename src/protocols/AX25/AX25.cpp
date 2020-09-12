#include "AX25.h"
#if !defined(RADIOLIB_EXCLUDE_AX25)

AX25Frame::AX25Frame(const char* destCallsign, uint8_t destSSID, const char* srcCallsign, uint8_t srcSSID, uint8_t control)
: AX25Frame(destCallsign, destSSID, srcCallsign, srcSSID, control, 0, NULL, 0) {

}

AX25Frame::AX25Frame(const char* destCallsign, uint8_t destSSID, const char* srcCallsign, uint8_t srcSSID, uint8_t control, uint8_t protocolID, const char* info)
  : AX25Frame(destCallsign, destSSID, srcCallsign, srcSSID, control, protocolID, (uint8_t*)info, strlen(info)) {

}

AX25Frame::AX25Frame(const char* destCallsign, uint8_t destSSID, const char* srcCallsign, uint8_t srcSSID, uint8_t control, uint8_t protocolID, uint8_t* info, uint16_t infoLen) {
  // destination callsign/SSID
  memcpy(this->destCallsign, destCallsign, strlen(destCallsign));
  this->destCallsign[strlen(destCallsign)] = '\0';
  this->destSSID = destSSID;

  // source callsign/SSID
  memcpy(this->srcCallsign, srcCallsign, strlen(srcCallsign));
  this->srcCallsign[strlen(srcCallsign)] = '\0';
  this->srcSSID = srcSSID;

  // set repeaters
  this->numRepeaters = 0;
  #ifndef RADIOLIB_STATIC_ONLY
    this->repeaterCallsigns = NULL;
    this->repeaterSSIDs = NULL;
  #endif

  // control field
  this->control = control;

  // sequence numbers
  this->rcvSeqNumber = 0;
  this->sendSeqNumber = 0;

  // PID field
  this->protocolID = protocolID;

  // info field
  this->infoLen = infoLen;
  if(infoLen > 0) {
    #ifndef RADIOLIB_STATIC_ONLY
      this->info = new uint8_t[infoLen];
    #endif
    memcpy(this->info, info, infoLen);
  }
}

AX25Frame::AX25Frame(const AX25Frame& frame) {
  *this = frame;
}

AX25Frame::~AX25Frame() {
  #ifndef RADIOLIB_STATIC_ONLY
    // deallocate info field
    if(infoLen > 0) {
      delete[] this->info;
    }

    // deallocate repeaters
    if(this->numRepeaters > 0) {
      for(uint8_t i = 0; i < this->numRepeaters; i++) {
        delete[] this->repeaterCallsigns[i];
      }
      delete[] this->repeaterCallsigns;
      delete[] this->repeaterSSIDs;
    }
  #endif
}

AX25Frame& AX25Frame::operator=(const AX25Frame& frame) {
  // destination callsign/SSID
  memcpy(this->destCallsign, frame.destCallsign, strlen(frame.destCallsign));
  this->destCallsign[strlen(frame.destCallsign)] = '\0';
  this->destSSID = frame.destSSID;

  // source callsign/SSID
  memcpy(this->srcCallsign, frame.srcCallsign, strlen(frame.srcCallsign));
  this->srcCallsign[strlen(frame.srcCallsign)] = '\0';
  this->srcSSID = frame.srcSSID;

  // repeaters
  this->numRepeaters = frame.numRepeaters;
  for(uint8_t i = 0; i < this->numRepeaters; i++) {
    memcpy(this->repeaterCallsigns[i], frame.repeaterCallsigns[i], strlen(frame.repeaterCallsigns[i]));
  }
  memcpy(this->repeaterSSIDs, frame.repeaterSSIDs, this->numRepeaters);

  // control field
  this->control = frame.control;

  // sequence numbers
  this->rcvSeqNumber = frame.rcvSeqNumber;
  this->sendSeqNumber = frame.sendSeqNumber;

  // PID field
  this->protocolID = frame.protocolID;

  // info field
  this->infoLen = frame.infoLen;
  memcpy(this->info, frame.info, this->infoLen);

  return(*this);
}

int16_t AX25Frame::setRepeaters(char** repeaterCallsigns, uint8_t* repeaterSSIDs, uint8_t numRepeaters) {
  // check number of repeaters
  if((numRepeaters < 1) || (numRepeaters > 8)) {
    return(ERR_INVALID_NUM_REPEATERS);
  }

  // check repeater configuration
  if((repeaterCallsigns == NULL) || (repeaterSSIDs == NULL)) {
    return(ERR_INVALID_NUM_REPEATERS);
  }
  for(uint16_t i = 0; i < numRepeaters; i++) {
    if(strlen(repeaterCallsigns[i]) > AX25_MAX_CALLSIGN_LEN) {
      return(ERR_INVALID_REPEATER_CALLSIGN);
    }
  }

  // create buffers
  #ifndef RADIOLIB_STATIC_ONLY
    this->repeaterCallsigns = new char*[numRepeaters];
    for(uint8_t i = 0; i < numRepeaters; i++) {
      this->repeaterCallsigns[i] = new char[strlen(repeaterCallsigns[i])];
    }
    this->repeaterSSIDs = new uint8_t[numRepeaters];
  #endif

  // copy data
  this->numRepeaters = numRepeaters;
  for(uint8_t i = 0; i < numRepeaters; i++) {
    memcpy(this->repeaterCallsigns[i], repeaterCallsigns[i], strlen(repeaterCallsigns[i]));
  }
  memcpy(this->repeaterSSIDs, repeaterSSIDs, numRepeaters);

  return(ERR_NONE);
}

void AX25Frame::setRecvSequence(uint8_t seqNumber) {
  this->rcvSeqNumber = seqNumber;
}

void AX25Frame::setSendSequence(uint8_t seqNumber) {
  this->sendSeqNumber = seqNumber;
}

AX25Client::AX25Client(PhysicalLayer* phy) {
  _phy = phy;
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  _audio = nullptr;
  #endif
}

#if !defined(RADIOLIB_EXCLUDE_AFSK)
AX25Client::AX25Client(AFSKClient* audio) {
  _phy = audio->_phy;
  _audio = audio;
}
#endif

int16_t AX25Client::begin(const char* srcCallsign, uint8_t srcSSID, uint8_t preambleLen) {
  // set source SSID
  _srcSSID = srcSSID;

  // check source callsign length (6 characters max)
  if(strlen(srcCallsign) > AX25_MAX_CALLSIGN_LEN) {
    return(ERR_INVALID_CALLSIGN);
  }

  // copy callsign
  memcpy(_srcCallsign, srcCallsign, strlen(srcCallsign));
  _srcCallsign[strlen(srcCallsign)] = '\0';

  // save preamble length
  _preambleLen = preambleLen;

  // set module frequency deviation to 0 if using FSK
  int16_t state = ERR_NONE;
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio == nullptr) {
    state = _phy->setFrequencyDeviation(0);
    RADIOLIB_ASSERT(state);

    state = _phy->setEncoding(0);
  }
  #endif
  return(state);
}

int16_t AX25Client::transmit(const char* str, const char* destCallsign, uint8_t destSSID) {
  // create control field
  uint8_t controlField = AX25_CONTROL_U_UNNUMBERED_INFORMATION | AX25_CONTROL_POLL_FINAL_DISABLED | AX25_CONTROL_UNNUMBERED_FRAME;

  // build the frame
  AX25Frame frame(destCallsign, destSSID, _srcCallsign, _srcSSID, controlField, AX25_PID_NO_LAYER_3, (uint8_t*)str, strlen(str));

  // send Unnumbered Information frame
  return(sendFrame(&frame));
}

int16_t AX25Client::sendFrame(AX25Frame* frame) {
  // check destination callsign length (6 characters max)
  if(strlen(frame->destCallsign) > AX25_MAX_CALLSIGN_LEN) {
    return(ERR_INVALID_CALLSIGN);
  }

  // check repeater configuration
  #ifndef RADIOLIB_STATIC_ONLY
    if(!(((frame->repeaterCallsigns == NULL) && (frame->repeaterSSIDs == NULL) && (frame->numRepeaters == 0)) ||
         ((frame->repeaterCallsigns != NULL) && (frame->repeaterSSIDs != NULL) && (frame->numRepeaters != 0)))) {
      return(ERR_INVALID_NUM_REPEATERS);
    }
    for(uint16_t i = 0; i < frame->numRepeaters; i++) {
      if(strlen(frame->repeaterCallsigns[i]) > AX25_MAX_CALLSIGN_LEN) {
        return(ERR_INVALID_REPEATER_CALLSIGN);
      }
    }
  #endif

  // calculate frame length without FCS (destination address, source address, repeater addresses, control, PID, info)
  size_t frameBuffLen = ((2 + frame->numRepeaters)*(AX25_MAX_CALLSIGN_LEN + 1)) + 1 + 1 + frame->infoLen;
  // create frame buffer without preamble, start or stop flags
  #ifndef RADIOLIB_STATIC_ONLY
    uint8_t* frameBuff = new uint8_t[frameBuffLen + 2];
  #else
    uint8_t frameBuff[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif
  uint8_t* frameBuffPtr = frameBuff;

  // set destination callsign - all address field bytes are shifted by one bit to make room for HDLC address extension bit
  memset(frameBuffPtr, ' ' << 1, AX25_MAX_CALLSIGN_LEN);
  for(size_t i = 0; i < strlen(frame->destCallsign); i++) {
    *(frameBuffPtr + i) = frame->destCallsign[i] << 1;
  }
  frameBuffPtr += AX25_MAX_CALLSIGN_LEN;

  // set destination SSID
  *(frameBuffPtr++) = AX25_SSID_COMMAND_DEST | AX25_SSID_RESERVED_BITS | (frame->destSSID & 0x0F) << 1 | AX25_SSID_HDLC_EXTENSION_CONTINUE;

  // set source callsign - all address field bytes are shifted by one bit to make room for HDLC address extension bit
  memset(frameBuffPtr, ' ' << 1, AX25_MAX_CALLSIGN_LEN);
  for(size_t i = 0; i < strlen(frame->srcCallsign); i++) {
    *(frameBuffPtr + i) = frame->srcCallsign[i] << 1;
  }
  frameBuffPtr += AX25_MAX_CALLSIGN_LEN;

  // set source SSID
  *(frameBuffPtr++) = AX25_SSID_COMMAND_SOURCE | AX25_SSID_RESERVED_BITS | (frame->srcSSID & 0x0F) << 1 | AX25_SSID_HDLC_EXTENSION_END;

  // set repeater callsigns
  for(uint16_t i = 0; i < frame->numRepeaters; i++) {
    memset(frameBuffPtr, ' ' << 1, AX25_MAX_CALLSIGN_LEN);
    for(size_t j = 0; j < strlen(frame->repeaterCallsigns[i]); j++) {
      *(frameBuffPtr + j) = frame->repeaterCallsigns[i][j] << 1;
    }
    frameBuffPtr += AX25_MAX_CALLSIGN_LEN;
    *(frameBuffPtr++) = AX25_SSID_HAS_NOT_BEEN_REPEATED | AX25_SSID_RESERVED_BITS | (frame->repeaterSSIDs[i] & 0x0F) << 1 | AX25_SSID_HDLC_EXTENSION_CONTINUE;
  }

  // set HDLC extension end bit
  *frameBuffPtr |= AX25_SSID_HDLC_EXTENSION_END;

  // set sequence numbers of the frames that have it
  uint8_t controlField = frame->control;
  if((frame->control & 0x01) == 0) {
    // information frame, set both sequence numbers
    controlField |= frame->rcvSeqNumber << 5;
    controlField |= frame->sendSeqNumber << 1;
  } else if((frame->control & 0x02) == 0) {
    // supervisory frame, set only receive sequence number
    controlField |= frame->rcvSeqNumber << 5;
  }

  // set control field
  *(frameBuffPtr++) = controlField;

  // set PID field of the frames that have it
  if(frame->protocolID != 0x00) {
    *(frameBuffPtr++) = frame->protocolID;
  }

  // set info field of the frames that have it
  if(frame->infoLen > 0) {
    memcpy(frameBuffPtr, frame->info, frame->infoLen);
    frameBuffPtr += frame->infoLen;
  }

  // flip bit order
  for(size_t i = 0; i < frameBuffLen; i++) {
    frameBuff[i] = flipBits(frameBuff[i]);
  }

  // calculate FCS
  uint16_t fcs = getFrameCheckSequence(frameBuff, frameBuffLen);
  *(frameBuffPtr++) = (uint8_t)((fcs >> 8) & 0xFF);
  *(frameBuffPtr++) = (uint8_t)(fcs & 0xFF);

  // prepare buffer for the final frame (stuffed, with added preamble + flags and NRZI-encoded)
  #ifndef RADIOLIB_STATIC_ONLY
    // worst-case scenario: sequence of 1s, will have 120% of the original length, stuffed frame also includes both flags
    uint8_t* stuffedFrameBuff = new uint8_t[_preambleLen + 1 + (6*frameBuffLen)/5 + 2];
  #else
    uint8_t stuffedFrameBuff[RADIOLIB_STATIC_ARRAY_SIZE];
  #endif

  // initialize buffer to all zeros
  memset(stuffedFrameBuff, 0x00, _preambleLen + 1 + (6*frameBuffLen)/5 + 2);

  // stuff bits (skip preamble and both flags)
  uint16_t stuffedFrameBuffLenBits = 8*(_preambleLen + 1);
  uint8_t count = 0;
  for(size_t i = 0; i < frameBuffLen + 2; i++) {
    for(int8_t shift = 7; shift >= 0; shift--) {
      uint16_t stuffedFrameBuffPos = stuffedFrameBuffLenBits + 7 - 2*(stuffedFrameBuffLenBits%8);
      if((frameBuff[i] >> shift) & 0x01) {
        // copy 1 and increment counter
        SET_BIT_IN_ARRAY(stuffedFrameBuff, stuffedFrameBuffPos);
        stuffedFrameBuffLenBits++;
        count++;

        // check 5 consecutive 1s
        if(count == 5) {
          // get the new position in stuffed frame
          stuffedFrameBuffPos = stuffedFrameBuffLenBits + 7 - 2*(stuffedFrameBuffLenBits%8);

          // insert 0 and reset counter
          CLEAR_BIT_IN_ARRAY(stuffedFrameBuff, stuffedFrameBuffPos);
          stuffedFrameBuffLenBits++;
          count = 0;
        }

      } else {
        // copy 0 and reset counter
        CLEAR_BIT_IN_ARRAY(stuffedFrameBuff, stuffedFrameBuffPos);
        stuffedFrameBuffLenBits++;
        count = 0;
      }

    }
  }

  // deallocate memory
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] frameBuff;
  #endif

  // set preamble bytes and start flag field
  for(uint16_t i = 0; i < _preambleLen + 1; i++) {
    stuffedFrameBuff[i] = AX25_FLAG;
  }

  // get stuffed frame length in bytes
  size_t stuffedFrameBuffLen = stuffedFrameBuffLenBits/8 + 1;
  uint8_t trailingLen = stuffedFrameBuffLenBits % 8;

  // set end flag field (may be split into two bytes due to misalignment caused by extra stuffing bits)
  if(trailingLen != 0) {
    stuffedFrameBuffLen++;
    stuffedFrameBuff[stuffedFrameBuffLen - 2] = AX25_FLAG >> trailingLen;
    stuffedFrameBuff[stuffedFrameBuffLen - 1] = AX25_FLAG << (8 - trailingLen);
  } else {
    stuffedFrameBuff[stuffedFrameBuffLen - 1] = AX25_FLAG;
  }

  // convert to NRZI
  for(size_t i = _preambleLen + 1; i < stuffedFrameBuffLen*8; i++) {
    size_t currBitPos = i + 7 - 2*(i%8);
    size_t prevBitPos = (i - 1) + 7 - 2*((i - 1)%8);
    if(TEST_BIT_IN_ARRAY(stuffedFrameBuff, currBitPos)) {
      // bit is 1, no change, copy previous bit
      if(TEST_BIT_IN_ARRAY(stuffedFrameBuff, prevBitPos)) {
        SET_BIT_IN_ARRAY(stuffedFrameBuff, currBitPos);
      } else {
        CLEAR_BIT_IN_ARRAY(stuffedFrameBuff, currBitPos);
      }

    } else {
      // bit is 0, transition, copy inversion of the previous bit
      if(TEST_BIT_IN_ARRAY(stuffedFrameBuff, prevBitPos)) {
        CLEAR_BIT_IN_ARRAY(stuffedFrameBuff, currBitPos);
      } else {
        SET_BIT_IN_ARRAY(stuffedFrameBuff, currBitPos);
      }
    }
  }

  // transmit
  int16_t state = ERR_NONE;
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  if(_audio != nullptr) {
    _phy->transmitDirect();

    // iterate over all bytes in the buffer
    for(uint32_t i = 0; i < stuffedFrameBuffLen; i++) {

      // check each bit
      for(uint16_t mask = 0x80; mask >= 0x01; mask >>= 1) {
        uint32_t start = Module::micros();
        if(stuffedFrameBuff[i] & mask) {
          _audio->tone(AX25_AFSK_MARK, false);
        } else {
          _audio->tone(AX25_AFSK_SPACE, false);
        }
        while(Module::micros() - start < AX25_AFSK_TONE_DURATION) {
          Module::yield();
        }
      }

    }

    _audio->noTone();

  } else {
  #endif
    state = _phy->transmit(stuffedFrameBuff, stuffedFrameBuffLen);
  #if !defined(RADIOLIB_EXCLUDE_AFSK)
  }
  #endif

  // deallocate memory
  #ifndef RADIOLIB_STATIC_ONLY
    delete[] stuffedFrameBuff;
  #endif

  return(state);
}

/*
  CCITT CRC implementation based on https://github.com/kicksat/ax25

  Licensed under Creative Commons Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/
*/
uint16_t AX25Client::getFrameCheckSequence(uint8_t* buff, size_t len) {
  uint8_t outBit;
  uint16_t mask;
  uint16_t shiftReg = CRC_CCITT_INIT;

  for(size_t i = 0; i < len; i++) {
    for(uint8_t b = 0x80; b > 0x00; b /= 2) {
      outBit = (shiftReg & 0x01) ? 0x01 : 0x00;
      shiftReg >>= 1;
      mask = XOR((buff[i] & b), outBit) ? CRC_CCITT_POLY_REVERSED : 0x0000;
      shiftReg ^= mask;
    }
  }

  return(flipBits16(~shiftReg));
}

uint8_t AX25Client::flipBits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

uint16_t AX25Client::flipBits16(uint16_t i) {
  i = (i & 0xFF00) >> 8 | (i & 0x00FF) << 8;
  i = (i & 0xF0F0) >> 4 | (i & 0x0F0F) << 4;
  i = (i & 0xCCCC) >> 2 | (i & 0x3333) << 2;
  i = (i & 0xAAAA) >> 1 | (i & 0x5555) << 1;
  return i;
}

#endif
