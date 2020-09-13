#ifndef _RADIOLIB_PHYSICAL_LAYER_H
#define _RADIOLIB_PHYSICAL_LAYER_H

#include "../../TypeDef.h"

/*!
  \class PhysicalLayer

  \brief Provides common interface for protocols that run on %LoRa/FSK modules, such as RTTY or LoRaWAN. Also extracts some common
  module-independent methods. Using this interface class allows to use the protocols on various modules without much code duplicity.
  Because this class is used mainly as interface, all of its virtual members must be implemented in the module class.
*/
class PhysicalLayer {
  public:

    // constructor

    /*!
      \brief Default constructor.

      \param freqStep Frequency step of the synthesizer in Hz.

      \param maxPacketLength Maximum length of packet that can be received by the module-
    */
    PhysicalLayer(float freqStep, size_t maxPacketLength);

    // basic methods

    /*!
      \brief Arduino Flash String transmit method.

      \param str Pointer to Arduino Flash String that will be transmitted.

      \param addr Node address to transmit the packet to. Only used in FSK mode.

      \returns \ref status_codes
    */
    int16_t transmit(__FlashStringHelper* fstr, uint8_t addr = 0);

    /*!
      \brief Arduino String transmit method.

      \param str Address of Arduino string that will be transmitted.

      \param addr Node address to transmit the packet to. Only used in FSK mode.

      \returns \ref status_codes
    */
    int16_t transmit(String& str, uint8_t addr = 0);

    /*!
      \brief C-string transmit method.

      \param str C-string that will be transmitted.

      \param addr Node address to transmit the packet to. Only used in FSK mode.

      \returns \ref status_codes
    */
    int16_t transmit(const char* str, uint8_t addr = 0);

    /*!
      \brief Binary transmit method. Must be implemented in module class.

      \param data Binary data that will be transmitted.

      \param len Length of binary data to transmit (in bytes).

      \param addr Node address to transmit the packet to. Only used in FSK mode.

      \returns \ref status_codes
    */
    virtual int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0) = 0;

    /*!
      \brief Arduino String receive method.

      \param str Address of Arduino String to save the received data.

      \param len Expected number of characters in the message. Leave as 0 if expecting a unknown size packet

      \returns \ref status_codes
    */
    int16_t receive(String& str, size_t len = 0);

    /*!
      \brief Sets module to standby.

      \returns \ref status_codes
    */
    virtual int16_t standby() = 0;

    /*!
      \brief Binary receive method. Must be implemented in module class.

      \param data Pointer to array to save the received binary data.

      \param len Number of bytes that will be received. Must be known in advance for binary transmissions.

      \returns \ref status_codes
    */
    virtual int16_t receive(uint8_t* data, size_t len) = 0;

    /*!
      \brief Interrupt-driven Arduino String transmit method. Unlike the standard transmit method, this one is non-blocking.
      Interrupt pin will be activated when transmission finishes.

      \param str Address of Arduino String that will be transmitted.

      \param addr Node address to transmit the packet to. Only used in FSK mode.

      \returns \ref status_codes
    */
    int16_t startTransmit(String& str, uint8_t addr = 0);

    /*!
      \brief Interrupt-driven Arduino String transmit method. Unlike the standard transmit method, this one is non-blocking.
      Interrupt pin will be activated when transmission finishes.

      \param str C-string that will be transmitted.

      \param addr Node address to transmit the packet to. Only used in FSK mode.

      \returns \ref status_codes
    */
    int16_t startTransmit(const char* str, uint8_t addr = 0);

    /*!
      \brief Interrupt-driven binary transmit method.

      \param data Binary data that will be transmitted.

      \param len Length of binary data to transmit (in bytes).

      \param addr Node address to transmit the packet to. Only used in FSK mode.

      \returns \ref status_codes
    */
    virtual int16_t startTransmit(uint8_t* data, size_t len, uint8_t addr = 0) = 0;

    /*!
      \brief Reads data that was received after calling startReceive method.

      \param str Address of Arduino String to save the received data.

      \param len Expected number of characters in the message.

      \returns \ref status_codes
    */
    int16_t readData(String& str, size_t len = 0);

    /*!
      \brief Reads data that was received after calling startReceive method.

      \param data Pointer to array to save the received binary data.

      \param len Number of bytes that will be received. Must be known in advance for binary transmissions.

      \returns \ref status_codes
    */
    virtual int16_t readData(uint8_t* data, size_t len) = 0;

    /*!
      \brief Enables direct transmission mode on pins DIO1 (clock) and DIO2 (data). Must be implemented in module class.
      While in direct mode, the module will not be able to transmit or receive packets. Can only be activated in FSK mode.

      \param frf 24-bit raw frequency value to start transmitting at. Required for quick frequency shifts in RTTY.

      \returns \ref status_codes
    */
    virtual int16_t transmitDirect(uint32_t frf = 0) = 0;

    /*!
      \brief Enables direct reception mode on pins DIO1 (clock) and DIO2 (data). Must be implemented in module class.
      While in direct mode, the module will not be able to transmit or receive packets. Can only be activated in FSK mode.

      \returns \ref status_codes
    */
    virtual int16_t receiveDirect() = 0;

    // configuration methods

    /*!
      \brief Sets FSK frequency deviation from carrier frequency. Allowed values depend on bit rate setting and must be lower than 200 kHz.
      Only available in FSK mode. Must be implemented in module class.

      \param freqDev Frequency deviation to be set (in kHz).

      \returns \ref status_codes
    */
    virtual int16_t setFrequencyDeviation(float freqDev) = 0;

    /*!
      \brief Sets GFSK data shaping. Only available in FSK mode. Must be implemented in module class.

      \param sh Shaping to be set. See \ref config_shaping for possible values.

      \returns \ref status_codes
    */
    virtual int16_t setDataShaping(uint8_t sh) = 0;

    /*!
      \brief Sets FSK data encoding. Only available in FSK mode. Must be implemented in module class.

      \param enc Encoding to be used. See \ref config_encoding for possible values.

      \returns \ref status_codes
    */
    virtual int16_t setEncoding(uint8_t encoding) = 0;

    /*!
      \brief Gets the module frequency step size that was set in constructor.

      \returns Synthesizer frequency step size in Hz.
    */
    float getFreqStep() const;

    /*!
     \brief Query modem for the packet length of received payload. Must be implemented in module class.

     \param update Update received packet length. Will return cached value when set to false.

     \returns Length of last received packet in bytes.
   */
   virtual size_t getPacketLength(bool update = true) = 0;

   /*!
    \brief Get truly random number in range 0 - max.

    \param max The maximum value of the random number (non-inclusive).

    \returns Random number.
   */
   int32_t random(int32_t max);

   /*!
    \brief Get truly random number in range min - max.

    \param min The minimum value of the random number (inclusive).

    \param max The maximum value of the random number (non-inclusive).
   
    \returns Random number.
   */
   int32_t random(int32_t min, int32_t max);

   /*!
    \brief Get one truly random byte from RSSI noise. Must be implemented in module class.

    \returns TRNG byte.
  */
   virtual uint8_t random() = 0;

#ifndef RADIOLIB_GODMODE
  private:
#endif
    float _freqStep;
    size_t _maxPacketLength;
};

#endif
