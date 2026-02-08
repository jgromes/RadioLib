#include "ADSB.h"

#if !RADIOLIB_EXCLUDE_ADSB

ADSBClient::ADSBClient(PhysicalLayer* phy) {
  phyLayer = phy;
}

int16_t ADSBClient::begin() {
  // set what is supported by PHY
  int16_t state = phyLayer->setFrequency(RADIOLIB_ADSB_CARRIER_FREQUENCY);
  RADIOLIB_ASSERT(state);

  state = phyLayer->setPreambleLength(16);
  RADIOLIB_ASSERT(state);
  
  state = phyLayer->setSyncWord(NULL, 0);
  RADIOLIB_ASSERT(state);

  state = phyLayer->setEncoding(RADIOLIB_ENCODING_MANCHESTER_INV);
  return(state);
}

int16_t ADSBClient::decode(const uint8_t in[RADIOLIB_ADSB_FRAME_LEN_BYTES], ADSBFrame* out) {
  RADIOLIB_ASSERT_PTR(out);

  // get the basic information
  out->downlinkFormat = (in[0] & 0xF8) >> 3;
  out->capability = in[0] & 0x07;
  for(int i = 0; i < RADIOLIB_ADSB_FRAME_ICAO_LEN_BYTES; i++) {
    out->icao[i] = in[RADIOLIB_ADSB_FRAME_ICAO_POS + i];
  }

  // lookup table to avoid a whole bunch of if-else statements
  const ADSBMessageType msgTypeLut[] = {
    ADSBMessageType::RESERVED,
    ADSBMessageType::AIRCRAFT_ID, ADSBMessageType::AIRCRAFT_ID,
    ADSBMessageType::AIRCRAFT_ID, ADSBMessageType::AIRCRAFT_ID, 
    ADSBMessageType::SURFACE_POS, ADSBMessageType::SURFACE_POS, 
    ADSBMessageType::SURFACE_POS, ADSBMessageType::SURFACE_POS, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_POS_ALT_BARO, ADSBMessageType::AIRBORNE_POS_ALT_BARO, 
    ADSBMessageType::AIRBORNE_VEL, 
    ADSBMessageType::AIRBORNE_POS_ALT_GNSS,
    ADSBMessageType::AIRBORNE_POS_ALT_GNSS,
    ADSBMessageType::AIRBORNE_POS_ALT_GNSS,
    ADSBMessageType::RESERVED, ADSBMessageType::RESERVED,
    ADSBMessageType::RESERVED, ADSBMessageType::RESERVED, ADSBMessageType::RESERVED,
    ADSBMessageType::AIRCRAFT_STATUS,
    ADSBMessageType::TARGET_STATE,
    ADSBMessageType::RESERVED,
    ADSBMessageType::AIRCRAFT_OPS_STATUS,
  };

  // get the message type and then the message itself
  uint8_t typeCode = (in[RADIOLIB_ADSB_FRAME_MESSAGE_POS] & (0x1F << 3)) >> 3;
  RADIOLIB_DEBUG_PROTOCOL_PRINTLN("ADS-B Type Code = %d", typeCode);
  out->messageType = msgTypeLut[typeCode];
  for(int i = 0; i < RADIOLIB_ADSB_FRAME_MESSAGE_LEN_BYTES; i++) {
    out->message[i] = in[RADIOLIB_ADSB_FRAME_MESSAGE_POS + i];
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t ADSBClient::parseCallsign(ADSBFrame* in, char callsign[RADIOLIB_ADSB_CALLSIGN_LEN], ADSBAircraftCategory* cat) {
  RADIOLIB_ASSERT_PTR(in);

  if(in->messageType != ADSBMessageType::AIRCRAFT_ID) {
    return(RADIOLIB_ERR_ADSB_INVALID_MSG_TYPE);
  }

  // precomputed bitshifts
  callsign[0] = (in->message[1] & (0x3F << 2)) >> 2;
  callsign[1] = ((in->message[1] & (0x03 >> 0)) << 4) | ((in->message[2] & (0x0F << 4)) >> 4);
  callsign[2] = ((in->message[2] & (0x0F << 0)) << 2) | ((in->message[3] & (0x03 << 6)) >> 6);
  callsign[3] = in->message[3] & 0x3F;
  callsign[4] = (in->message[4] & (0x3F << 2)) >> 2;
  callsign[5] = ((in->message[4] & (0x03 >> 0)) << 4) | ((in->message[5] & (0x0F << 4)) >> 4);
  callsign[6] = ((in->message[5] & (0x0F << 0)) << 2) | ((in->message[6] & (0x03 << 6)) >> 6);
  callsign[7] = in->message[6] & 0x3F;
  callsign[RADIOLIB_ADSB_CALLSIGN_LEN - 1] = '\0';

  // convert to ASCII
  for(int i = 0; i < RADIOLIB_ADSB_CALLSIGN_LEN - 1; i++) {
    if((callsign[i] >= 1) && (callsign[i] <= 26)) {
      callsign[i] += '@';
    } else if(callsign[i] == 32) {
      callsign[i] = ' ';
    } else if(!((callsign[i] >= '0') && (callsign[i] <= '9'))) {
      callsign[i] = '\0';
    }
  }

  // only continue processing if category was requested by user
  if(!cat) { return(RADIOLIB_ERR_NONE);}
  
  uint8_t type = (in->message[0] & 0xF8) >> 3;
  uint8_t category = in->message[0] & 0x03;
  if(type < 2) {
    *cat = ADSBAircraftCategory::RESERVED;
    return(RADIOLIB_ERR_NONE);
  } else if(category == 0) {
    *cat = ADSBAircraftCategory::NONE;
    return(RADIOLIB_ERR_NONE);
  }

  // lookup table to avoid a whole bunch of if-else statements
  const ADSBAircraftCategory catLut[] = {
    ADSBAircraftCategory::SURFACE_EMERGENCY,
    ADSBAircraftCategory::RESERVED,
    ADSBAircraftCategory::SURFACE_SERVICE,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GROUND_OBSTRUCTION,
    ADSBAircraftCategory::GLIDER,
    ADSBAircraftCategory::LIGHTER_THAN_AIR,
    ADSBAircraftCategory::PARACHUTIST,
    ADSBAircraftCategory::ULTRALIGHT_PARAGLIDER,
    ADSBAircraftCategory::RESERVED,
    ADSBAircraftCategory::UAV,
    ADSBAircraftCategory::SPACE_VEHICLE,
    ADSBAircraftCategory::LIGHT,
    ADSBAircraftCategory::MEDIUM_1,
    ADSBAircraftCategory::MEDIUM_2,
    ADSBAircraftCategory::HIGH_VORTEX,
    ADSBAircraftCategory::HEAVY,
    ADSBAircraftCategory::HIGH_PERFORMANCE,
    ADSBAircraftCategory::ROTORCRAFT,
  };

  // the index is parsed from the type and category
  // in real world values overflowing the lookup table should not appear,
  // but better to check anyway
  size_t index = (type - 2)*7 + (category - 1);
  if(index >= sizeof(catLut)/sizeof(catLut[0])) {
    return(RADIOLIB_ERR_ADSB_INVALID_CATEGORY);
  }

  *cat = catLut[index];
  return(RADIOLIB_ERR_NONE);
}

#endif
