#include "ISerial.h"

ISerial::ISerial(Module* mod) {
  _mod = mod;
}

void ISerial::begin(long speed) {
  _mod->ModuleSerial->begin(speed);
}

bool ISerial::listen() {
#ifdef RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  return true;
#else
  return(_mod->ModuleSerial->listen());
#endif
}

void ISerial::end() {
  _mod->ModuleSerial->end();
}

bool ISerial::isListening() {
#ifdef RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  return true;
#else
  return(_mod->ModuleSerial->isListening());
#endif
}

bool ISerial::stopListening() {
#ifdef RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  return true;
#else
  return(_mod->ModuleSerial->stopListening());
#endif
}

bool ISerial::overflow() {
#ifdef RADIOLIB_SOFTWARE_SERIAL_UNSUPPORTED
  return false;
#else
  return(_mod->ModuleSerial->overflow());
#endif
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

size_t ISerial::print(const __FlashStringHelper *ifsh) {
  return(_mod->ModuleSerial->print(ifsh));
}

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

size_t ISerial::println(const __FlashStringHelper *ifsh) {
  return(_mod->ModuleSerial->println(ifsh));
}

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
