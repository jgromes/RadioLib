#include "Utils.h"
#include "Hal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

uint32_t rlb_reflect(uint32_t in, uint8_t bits) {
  uint32_t res = 0;
  for(uint8_t i = 0; i < bits; i++) {
    res |= (((in & ((uint32_t)1 << i)) >> i) << (bits - i - 1));
  }
  return(res);
}

void rlb_hexdump(const char* level, const uint8_t* data, size_t len, uint32_t offset, uint8_t width, bool be) {
  #if RADIOLIB_DEBUG
  size_t rem_len = len;
  for(size_t i = 0; i < len; i+=16) {
    char str[120];
    sprintf(str, "%08" PRIx32 ": ", (uint32_t)i+offset);
    size_t line_len = 16;
    if(rem_len < line_len) {
      line_len = rem_len;
    }
    for(size_t j = 0; j < line_len; j+=width) {
      if(width > 1) {
        int m = 0;
        int step = width/2;
        if(be) {
          step *= -1;
        }
        for(int32_t k = width - 1; k >= -width + 1; k+=step) {
          sprintf(&str[10 + (j+m)*3], "%02x ", data[i+j+k+m]);
          m++;
        }
      } else {
        sprintf(&str[10 + (j)*3], "%02x ", data[i+j]);
      }
    }
    for(size_t j = line_len; j < 16; j++) {
      sprintf(&str[10 + j*3], "   ");
    }
    str[58] = ' ';

    // at this point we need to start escaping "%" characters
    char* strPtr = &str[59];
    for(size_t j = 0; j < line_len; j++) {
      char c = data[i+j];
      if((c < ' ') || (c > '~')) {
        c = '.';
      } else if(c == '%') {
        *strPtr++ = '%';
      }
      sprintf(strPtr++, "%c", c);
      
    }
    for(size_t j = line_len; j < 16; j++) {
      sprintf(strPtr++, "   ");
    }
    if(level) {
      RADIOLIB_DEBUG_PRINT_LVL("", "%s", level);
    }
    RADIOLIB_DEBUG_PRINT("%s", str);
    RADIOLIB_DEBUG_PRINTLN();
    rem_len -= 16;
  }

  #else
  // outside of debug, this does nothing
  (void)level;
  (void)data;
  (void)len;
  (void)offset;
  (void)width;
  (void)be;

  #endif
}

#if RADIOLIB_DEBUG
// https://github.com/esp8266/Arduino/blob/65579d29081cb8501e4d7f786747bf12e7b37da2/cores/esp8266/Print.cpp#L50
size_t rlb_printf(bool ts, const char* format, ...) {
  va_list arg;
  va_start(arg, format);
  char temp[64];
  char* buffer = temp;
  RadioLibTime_t timestamp = rlb_time_us();
  unsigned long sec = timestamp/1000000UL;
  unsigned long usec = timestamp%1000000UL;
  size_t len_ts = 0;
  if(ts) { len_ts = snprintf(temp, sizeof(temp), "[%lu.%06lu] ", sec, usec); }
  size_t len_str = vsnprintf(&temp[len_ts], sizeof(temp) - len_ts, format, arg);
  size_t len = len_ts + len_str;
  va_end(arg);
  if (len > sizeof(temp) - 1) {
    buffer = new char[len + 1];
    if (!buffer) {
      return 0;
    }
    va_start(arg, format);
    if(ts) { len_ts = snprintf(buffer, len_ts + 1, "[%lu.%06lu] ", sec, usec); }
    vsnprintf(buffer + len_ts, len_str + 1, format, arg);
    va_end(arg);
  }
  #if defined(RADIOLIB_BUILD_ARDUINO)
  len = RADIOLIB_DEBUG_PORT.write(reinterpret_cast<const uint8_t*>(buffer), len);
  #else
  len = fwrite(buffer, sizeof(temp[0]), len, RADIOLIB_DEBUG_PORT);
  #endif
  if (buffer != temp) {
    delete[] buffer;
  }
  return len;
}
#endif
