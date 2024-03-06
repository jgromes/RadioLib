/*
  EEPROM.cpp -originally written by Ivan Grokhotkov as part of the esp8266 
              core for Arduino environment.
             -ported by Paolo Becchi to Esp32 from esp8266 EEPROM
             -Modified by Elochukwu Ifediora <ifedioraelochukwuc@gmail.com>
             -Converted to nvs lbernstone@gmail.com
             -Adapted for ESP32_RTC_EEPROM.cpp by Rop Gonggrijp

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "ESP32_RTC_EEPROM.h"
#include <nvs.h>
#include <esp_partition.h>
#include <esp_log.h>

// Wake up with a magic word in there that lets us detect that we're starting from reset or power-down. 
uint8_t RTC_DATA_ATTR EEPROMClass::_data[EEPROM_SIZE] = { 0x23, 0x42, 0x23, 0x42, 0x23, 0x42, 0x23, 0x42 };

size_t EEPROMClass::_size = EEPROM_SIZE;

bool EEPROMClass::_restored = false;


EEPROMClass::EEPROMClass(void)
{
}

EEPROMClass::EEPROMClass(uint32_t sector)
// Only for compatiility, no sectors in RTC RAM!
{
}

EEPROMClass::~EEPROMClass() {
}

bool EEPROMClass::fromNVS() {
  nvs_handle _handle;
  char _name[] = EEPROM_FLASH_PARTITION_NAME;

  esp_err_t res = nvs_open(_name, NVS_READWRITE, &_handle);
  if (res != ESP_OK) {
      log_e("Unable to open NVS namespace: %d", res);
      return false;
  }

  size_t key_size = 0;
  res = nvs_get_blob(_handle, _name, NULL, &key_size);
  if(res != ESP_OK && res != ESP_ERR_NVS_NOT_FOUND) {
      log_e("Unable to read NVS key: %d", res);
      return false;
  }
  if (key_size != _size) {
      log_e("EEPROM in NVS not same size as EEPROM in RTC RAM");
      return false;
  }
  nvs_get_blob(_handle, _name, _data, &key_size);
  return true;
}

bool EEPROMClass::toNVS() {
  nvs_handle _handle;
  char _name[] = EEPROM_FLASH_PARTITION_NAME;

  esp_err_t res = nvs_open(_name, NVS_READWRITE, &_handle);
  if (res != ESP_OK) {
      log_e("Unable to open NVS namespace: %d", res);
      return false;
  }

  res = nvs_set_blob(_handle, _name, _data, _size);
  if (res != ESP_OK) {
      log_e("Unable to write NVS key: %d", res);
      return false;
  }
  res = nvs_commit(_handle);
  if (res != ESP_OK) {
      log_e("Unable to commit NVS key: %d", res);
      return false;
  }
  nvs_close(_handle);
  return true;
}

bool EEPROMClass::wasRestored() {
  return _restored;
}

bool EEPROMClass::begin(size_t size) {
  if (!size || size > EEPROM_SIZE) {
    return false;
  }
  _size = size;
  // See if we just woke up from reset or power-off
  uint64_t* magic_word = (uint64_t*) _data;
  // magic_word reversed because ESP32 is little-endian
  if (*magic_word == 0x4223422342234223) {
    log_w("Woke up cold, trying recover RTC EEPROM contents from NVS backup.");
    // if we did, see if we have a saved copy in NVS (flash)
    if (!fromNVS()) {
      log_w("No backup copy found, EEPROM starts with clean slate.");
      // If not, delete the magic word and start with a clean slate
      *magic_word = 0;
    } else {
      log_w("RTC EEPROM contents recovered from NVS backup.");
      _restored = true;
    }
  }
  return true;
}

void EEPROMClass::end()
{
}

uint8_t EEPROMClass::read(int address) {
  if (address < 0 || (size_t)address >= _size) {
    return 0;
  }
  return _data[address];
}

void EEPROMClass::write(int address, uint8_t value) {
  if (address < 0 || (size_t)address >= _size)
    return;
  _data[address] = value;
}

bool EEPROMClass::commit() {
  return true;
}

uint8_t * EEPROMClass::getDataPtr() {
  return &_data[0];
}

/*
   Get EEPROM total size in byte defined by the user
*/
uint16_t EEPROMClass::length () {
  return _size;
}

/*
   Read 'value' from 'address'
*/
uint8_t EEPROMClass::readByte (int address)
{
  uint8_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int8_t EEPROMClass::readChar (int address)
{
  int8_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint8_t EEPROMClass::readUChar (int address)
{
  uint8_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int16_t EEPROMClass::readShort (int address)
{
  int16_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint16_t EEPROMClass::readUShort (int address)
{
  uint16_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int32_t EEPROMClass::readInt (int address)
{
  int32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint32_t EEPROMClass::readUInt (int address)
{
  uint32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int32_t EEPROMClass::readLong (int address)
{
  int32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint32_t EEPROMClass::readULong (int address)
{
  uint32_t value = 0;
  return EEPROMClass::readAll (address, value);
}

int64_t EEPROMClass::readLong64 (int address)
{
  int64_t value = 0;
  return EEPROMClass::readAll (address, value);
}

uint64_t EEPROMClass::readULong64 (int address)
{
  uint64_t value = 0;
  return EEPROMClass::readAll (address, value);
}

float_t EEPROMClass::readFloat (int address)
{
  float_t value = 0;
  return EEPROMClass::readAll (address, value);
}

double_t EEPROMClass::readDouble (int address)
{
  double_t value = 0;
  return EEPROMClass::readAll (address, value);
}

bool EEPROMClass::readBool (int address)
{
  int8_t value = 0;
  return EEPROMClass::readAll (address, value) ? 1 : 0;
}

size_t EEPROMClass::readString (int address, char* value, size_t maxLen)
{
  if (!value)
    return 0;

  if (address < 0 || address + maxLen > _size)
    return 0;

  uint16_t len;
  for (len = 0; len <= _size; len++)
    if (_data[address + len] == 0)
      break;

  if (address + len > _size)
    return 0;

  if (len > maxLen)
    return 0; //Maybe return part of the string instead?

  memcpy((uint8_t*) value, _data + address, len);
  value[len] = 0;
  return len;
}

String EEPROMClass::readString (int address)
{
  if (address < 0 || address > _size)
    return String();

  uint16_t len;
  for (len = 0; len <= _size; len++)
    if (_data[address + len] == 0)
      break;

  if (address + len > _size)
    return String();

  char value[len+1];
  memcpy((uint8_t*) value, _data + address, len);
  value[len] = 0;
  return String(value);
}

size_t EEPROMClass::readBytes (int address, void* value, size_t maxLen)
{
  if (!value || !maxLen)
    return 0;

  if (address < 0 || address + maxLen > _size)
    return 0;

  memcpy((void*) value, _data + address, maxLen);
  return maxLen;
}

template <class T> T EEPROMClass::readAll (int address, T &value)
{
  if (address < 0 || address + sizeof(T) > _size)
    return value;

  memcpy((uint8_t*) &value, _data + address, sizeof(T));
  return value;
}

/*
   Write 'value' to 'address'
*/
size_t EEPROMClass::writeByte (int address, uint8_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeChar (int address, int8_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeUChar (int address, uint8_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeShort (int address, int16_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeUShort (int address, uint16_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeInt (int address, int32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeUInt (int address, uint32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeLong (int address, int32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeULong (int address, uint32_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeLong64 (int address, int64_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeULong64 (int address, uint64_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeFloat (int address, float_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeDouble (int address, double_t value)
{
  return EEPROMClass::writeAll (address, value);
}

size_t EEPROMClass::writeBool (int address, bool value)
{
  int8_t Bool;
  value ? Bool = 1 : Bool = 0;
  return EEPROMClass::writeAll (address, Bool);
}

size_t EEPROMClass::writeString (int address, const char* value)
{
  if (!value)
    return 0;

  if (address < 0 || address > _size)
    return 0;

  uint16_t len;
  for (len = 0; len <= _size; len++)
    if (value[len] == 0)
      break;

  if (address + len > _size)
    return 0;

  memcpy(_data + address, (const uint8_t*) value, len + 1);
  return strlen(value);
}

size_t EEPROMClass::writeString (int address, String value)
{
  return EEPROMClass::writeString (address, value.c_str());
}

size_t EEPROMClass::writeBytes (int address, const void* value, size_t len)
{
  if (!value || !len)
    return 0;

  if (address < 0 || address + len > _size)
    return 0;

  memcpy(_data + address, (const void*) value, len);
  return len;
}

template <class T> T EEPROMClass::writeAll (int address, const T &value)
{
  if (address < 0 || address + sizeof(T) > _size)
    return value;

  memcpy(_data + address, (const uint8_t*) &value, sizeof(T));
  return sizeof (value);
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
EEPROMClass EEPROM;
#endif
