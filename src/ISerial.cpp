#include "ISerial.h"

ISerial::ISerial(Module* mod) {
  _mod = mod;
}

void ISerial::begin(long speed) {
  _mod->ModuleSerial->begin(speed);
}

void ISerial::end() {
  _mod->ModuleSerial->end();
}


int ISerial::peek() {
  return(_mod->ModuleSerial->peek());
}

size_t ISerial::write(uint8_t b) {
  return(_mod->ModuleSerial->write(b));
}

int ISerial::read() {
  return(_mod->ModuleSerial->read());
}

int ISerial::available() {
  return(_mod->ModuleSerial->available());
}

void ISerial::flush() {
  _mod->ModuleSerial->flush();
}

#ifndef ARDUINO_ARCH_MEGAAVR
size_t ISerial::print(const __FlashStringHelper *ifsh) {
  return(_mod->ModuleSerial->print(ifsh));
}
#endif

size_t ISerial::print(const String &s) {
  return(_mod->ModuleSerial->print(s));
}

size_t ISerial::print(const char str[]) {
  return(_mod->ModuleSerial->print(str));
}

size_t ISerial::print(char c) {
  return(_mod->ModuleSerial->print(c));
}

size_t ISerial::print(unsigned char b, int base) {
  return(_mod->ModuleSerial->print(b, base));
}

size_t ISerial::print(int n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t ISerial::print(unsigned int n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t ISerial::print(long n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t ISerial::print(unsigned long n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t ISerial::print(double n, int digits) {
  return(_mod->ModuleSerial->print(n, digits));
}

size_t ISerial::print(const Printable& x) {
  return(_mod->ModuleSerial->print(x));
}

#ifndef ARDUINO_ARCH_MEGAAVR
size_t ISerial::println(const __FlashStringHelper *ifsh) {
  return(_mod->ModuleSerial->println(ifsh));
}
#endif

size_t ISerial::println(const String &s) {
  return(_mod->ModuleSerial->println(s));
}

size_t ISerial::println(const char str[]) {
  return(_mod->ModuleSerial->println(str));
}

size_t ISerial::println(char c) {
  return(_mod->ModuleSerial->println(c));
}

size_t ISerial::println(unsigned char b, int base) {
  return(_mod->ModuleSerial->println(b, base));
}

size_t ISerial::println(int n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t ISerial::println(unsigned int n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t ISerial::println(long n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t ISerial::println(unsigned long n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t ISerial::println(double n, int digits) {
  return(_mod->ModuleSerial->println(n, digits));
}

size_t ISerial::println(const Printable& x) {
  return(_mod->ModuleSerial->println(x));
}

size_t ISerial::println(void) {
  return(_mod->ModuleSerial->println());
}
