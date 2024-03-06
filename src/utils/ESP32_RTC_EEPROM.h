/*
  EEPROM.h -originally written by Ivan Grokhotkov as part of the esp8266 
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

#ifndef ESP32_RTC_EEPROM_h
#define ESP32_RTC_EEPROM_h

#define EEPROM_SIZE 2048

#ifndef EEPROM_FLASH_PARTITION_NAME
  #define EEPROM_FLASH_PARTITION_NAME "eeprom"
#endif

#include <Arduino.h>

class EEPROMClass {
  public:
    EEPROMClass(uint32_t sector);
    EEPROMClass(void);
    ~EEPROMClass(void);

    static bool begin(size_t size);
    static bool fromNVS();
    static bool toNVS();
    static bool wasRestored();
    static uint8_t read(int address);
    static void write(int address, uint8_t val);
    static uint16_t length();
    static bool commit();
    static void end();

    static uint8_t * getDataPtr();

    template<typename T>
    static T &get(int address, T &t) {
      if (address < 0 || address + sizeof(T) > _size)
        return t;

      memcpy((uint8_t*) &t, _data + address, sizeof(T));
      return t;
    }

    template<typename T>
    static const T &put(int address, const T &t) {
      if (address < 0 || address + sizeof(T) > _size)
        return t;

      memcpy(_data + address, (const uint8_t*) &t, sizeof(T));
      return t;
    }

    static uint8_t readByte(int address);
    static int8_t readChar(int address);
    static uint8_t readUChar(int address);
    static int16_t readShort(int address);
    static uint16_t readUShort(int address);
    static int32_t readInt(int address);
    static uint32_t readUInt(int address);
    static int32_t readLong(int address);
    static uint32_t readULong(int address);
    static int64_t readLong64(int address);
    static uint64_t readULong64(int address);
    static float_t readFloat(int address);
    static double_t readDouble(int address);
    static bool readBool(int address);
    static size_t readString(int address, char* value, size_t maxLen);
    static String readString(int address);
    static size_t readBytes(int address, void * value, size_t maxLen);
    template <class T> static T readAll (int address, T &);

    static size_t writeByte(int address, uint8_t value);
    static size_t writeChar(int address, int8_t value);
    static size_t writeUChar(int address, uint8_t value);
    static size_t writeShort(int address, int16_t value);
    static size_t writeUShort(int address, uint16_t value);
    static size_t writeInt(int address, int32_t value);
    static size_t writeUInt(int address, uint32_t value);
    static size_t writeLong(int address, int32_t value);
    static size_t writeULong(int address, uint32_t value);
    static size_t writeLong64(int address, int64_t value);
    static size_t writeULong64(int address, uint64_t value);
    static size_t writeFloat(int address, float_t value);
    static size_t writeDouble(int address, double_t value);
    static size_t writeBool(int address, bool value);
    static size_t writeString(int address, const char* value);
    static size_t writeString(int address, String value);
    static size_t writeBytes(int address, const void* value, size_t len);
    template <class T> static T writeAll (int address, const T &);

  protected:
    static size_t _size;
    static uint8_t RTC_DATA_ATTR _data[EEPROM_SIZE];
    static bool _restored;  
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
extern EEPROMClass EEPROM;
#endif

#endif
