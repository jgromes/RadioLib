#include "HC05.h"

HC05::HC05(Module* module) {
  _mod = module;
}

void HC05::begin(long speed) {
  _mod->baudrate = speed;
  _mod->init(USE_UART, INT_NONE);
}

bool HC05::listen() {
  return(_mod->ModuleSerial->listen());
}

void HC05::end() {
  _mod->ModuleSerial->end();
}

bool HC05::isListening() {
  return(_mod->ModuleSerial->isListening());
}

bool HC05::stopListening() {
  return(_mod->ModuleSerial->stopListening());
}

bool HC05::overflow() {
  return(_mod->ModuleSerial->overflow());
}

int HC05::peek() {
  return(_mod->ModuleSerial->peek());
}

size_t HC05::write(uint8_t b) {
  return(_mod->ModuleSerial->write(b));
}

int HC05::read() {
  return(_mod->ModuleSerial->read());
}

int HC05::available() {
  return(_mod->ModuleSerial->available());
}

void HC05::flush() {
  _mod->ModuleSerial->flush();
}

size_t HC05::print(const __FlashStringHelper *ifsh) {
  return(_mod->ModuleSerial->print(ifsh));
}

size_t HC05::print(const String &s) {
  return(_mod->ModuleSerial->print(s));
}

size_t HC05::print(const char str[]) {
  return(_mod->ModuleSerial->print(str));
}

size_t HC05::print(char c) {
  return(_mod->ModuleSerial->print(c));
}

size_t HC05::print(unsigned char b, int base) {
  return(_mod->ModuleSerial->print(b, base));
}

size_t HC05::print(int n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t HC05::print(unsigned int n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t HC05::print(long n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t HC05::print(unsigned long n, int base) {
  return(_mod->ModuleSerial->print(n, base));
}

size_t HC05::print(double n, int digits) {
  return(_mod->ModuleSerial->print(n, digits));
}

size_t HC05::print(const Printable& x) {
  return(_mod->ModuleSerial->print(x));
}

size_t HC05::println(const __FlashStringHelper *ifsh) {
  return(_mod->ModuleSerial->println(ifsh));
}

size_t HC05::println(const String &s) {
  return(_mod->ModuleSerial->println(s));
}

size_t HC05::println(const char str[]) {
  return(_mod->ModuleSerial->println(str));
}

size_t HC05::println(char c) {
  return(_mod->ModuleSerial->println(c));
}

size_t HC05::println(unsigned char b, int base) {
  return(_mod->ModuleSerial->println(b, base));
}

size_t HC05::println(int n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t HC05::println(unsigned int n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t HC05::println(long n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t HC05::println(unsigned long n, int base) {
  return(_mod->ModuleSerial->println(n, base));
}

size_t HC05::println(double n, int digits) {
  return(_mod->ModuleSerial->println(n, digits));
}

size_t HC05::println(const Printable& x) {
  return(_mod->ModuleSerial->println(x));
}

size_t HC05::println(void) {
  return(_mod->ModuleSerial->println());
}
