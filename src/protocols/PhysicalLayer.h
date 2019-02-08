#ifndef _RADIOLIB_PHYSICAL_LAYER_H
#define _RADIOLIB_PHYSICAL_LAYER_H

#include "TypeDef.h"

/*!
  \class PhysicalLayer

  \brief Provides common interface for protocols that run on %LoRa/FSK modules, such as RTTY or LoRaWAN. Also provides extracts some common
  module-independent methods. Using this interface class allows to use the protocols on various modules without much code duplicity.
  Because this class is used mainly as interface, all of its virtual members must be implemented in the module class.
*/
class PhysicalLayer {
  public:
  
    // constructor
    
    /*!
      \brief Default constructor.
      
      \param crysFreq Frequency of crystal oscillator inside the module in MHz.
      
      \param divExp Exponent of module frequency divider.
    */
    PhysicalLayer(float crysFreq, uint8_t divExp);
    
    // basic methods
    
    /*!
      \brief Arduino String transmit method. Will transmit Arduino String up to 255 characters long using %LoRa or up to 63 bytes using FSK modem.
      
      \param str Address of Arduino string that will be transmitted.
      
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      
      \returns \ref status_codes
    */
    int16_t transmit(String& str, uint8_t addr = 0);
    
    /*!
      \brief C-string transmit method. Will transmit C-string up to 255 characters long using %LoRa or up to 63 characters using FSK modem.
      
      \param str C-string that will be transmitted.
      
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      
      \returns \ref status_codes
    */
    int16_t transmit(const char* str, uint8_t addr = 0);
    
    /*!
      \brief Binary transmit method. Will transmit arbitrary binary data up to 255 bytes long using %LoRa or up to 63 bytes using FSK modem.
      Must be implemented in module class.
      
      \param data Binary data that will be transmitted.
      
      \param len Length of binary data to transmit (in bytes).
      
      \param addr Node address to transmit the packet to. Only used in FSK mode.
      
      \returns \ref status_codes
    */
    virtual int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0) = 0;
    
    /*!
      \brief Arduino String receive method. Will attempt to receive Arduino String up to 255 characters long using %LoRa or up to 63 characters using FSK modem.
      
      \param str Address of Arduino String to save the received data.
      
      \param len Expected number of characters in the message. Must be known in advance for %LoRa spreading factor 6.
      
      \returns \ref status_codes
    */
    int16_t receive(String& str, size_t len = 0);
    
    /*!
      \brief Binary receive method. Will attempt to receive arbitrary binary data up to 255 bytes long using %LoRa or up to 63 bytes using FSK modem.
      Must be implemented in module class.
      
      \param data Pointer to array to save the received binary data.
      
      \param len Number of bytes that will be received. Must be known in advance for binary transmissions.
      
      \returns \ref status_codes
    */
    virtual int16_t receive(uint8_t* data, size_t len) = 0;
    
    /*!
      \brief Enables direct transmission mode on pins DIO1 (clock) and DIO2 (data). Must be implemented in module class.
      While in direct mode, the module will not be able to transmit or receive packets. Can only be activated in FSK mode.
      
      \param FRF 24-bit raw frequency value to start transmitting at. Required for quick frequency shifts in RTTY.
      
      \returns \ref status_codes
    */
    virtual int16_t transmitDirect(uint32_t FRF = 0) = 0;
    
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
      \brief Gets the module crystal oscillator frequency that was set in constructor.
      
      \returns Crystal oscillator frequency in MHz.
    */
    float getCrystalFreq();
    
    /*!
      \brief Gets the module frequency divider exponent that was set in constructor.
      
      \returns Frequency divider exponent.
    */
    uint8_t getDivExponent();

  private:
    float _crystalFreq;
    uint8_t _divExponent;
};

#endif
