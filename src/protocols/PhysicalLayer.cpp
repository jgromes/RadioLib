#include "PhysicalLayer.h"

PhysicalLayer::PhysicalLayer(float crysFreq, uint8_t divExp) { 
  _crystalFreq = crysFreq;
  _divExponent = divExp;
}

int16_t PhysicalLayer::transmit(String& str, uint8_t addr) {
  return(transmit(str.c_str(), addr));
}

int16_t PhysicalLayer::transmit(const char* str, uint8_t addr) {
  return(transmit((uint8_t*)str, strlen(str), addr));
}

int16_t PhysicalLayer::receive(String& str, size_t len) {
  // create temporary array to store received data
  char* data = new char[len + 1];
  int16_t state = receive((uint8_t*)data, len);
  
  // if packet was received successfully, copy data into String
  if(state == ERR_NONE) {
    str = String(data);
  }
  
  delete[] data;
  return(state);
}

float PhysicalLayer::getCrystalFreq() {
  return(_crystalFreq);
}

uint8_t PhysicalLayer::getDivExponent() {
  return(_divExponent);
}
