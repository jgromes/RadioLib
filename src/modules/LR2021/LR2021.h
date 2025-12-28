#if !defined(RADIOLIB_LR2021_H)
#define RADIOLIB_LR2021_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

#include "../../Module.h"

#include "../../protocols/PhysicalLayer/PhysicalLayer.h"
#include "../LR11x0/LR_common.h"
#include "LR2021_commands.h"
#include "LR2021_types.h"

// LR2021 physical layer properties
#define RADIOLIB_LR2021_FREQUENCY_STEP_SIZE                     1.0
#define RADIOLIB_LR2021_MAX_PACKET_LENGTH                       255
#define RADIOLIB_LR2021_CRYSTAL_FREQ                            32.0
#define RADIOLIB_LR2021_DIV_EXPONENT                            25

/*!
  \class LR2021
  \brief 
*/
class LR2021: public PhysicalLayer, public LRxxxx {
  public:
    // introduce PhysicalLayer overloads
    using PhysicalLayer::transmit;
    using PhysicalLayer::receive;
    using PhysicalLayer::startTransmit;
    using PhysicalLayer::startReceive;
    using PhysicalLayer::readData;

    /*!
      \brief Default constructor.
      \param mod Instance of Module that will be used to communicate with the radio.
    */
    LR2021(Module* mod); // cppcheck-suppress noExplicitConstructor

    // basic methods

    /*!
      \brief Initialization method for LoRa modem.
      \param freq Carrier frequency in MHz. Defaults to 434.0 MHz.
      \param bw LoRa bandwidth in kHz. Defaults to 125.0 kHz.
      \param sf LoRa spreading factor. Defaults to 9.
      \param cr LoRa coding rate denominator. Defaults to 7 (coding rate 4/7). Allowed values range from 4 to 8. Note that a value of 4 means no coding,
      is undocumented and not recommended without your own FEC.
      \param syncWord 1-byte LoRa sync word. Defaults to RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE (0x12).
      \param power Output power in dBm. Defaults to 10 dBm.
      \param preambleLength LoRa preamble length in symbols. Defaults to 8 symbols.
      \param tcxoVoltage TCXO reference voltage to be set. Defaults to 1.6 V.
      If you are seeing -706/-707 error codes, it likely means you are using non-0 value for module with XTAL.
      To use XTAL, either set this value to 0, or set LR11x0::XTAL to true.
      \returns \ref status_codes
    */
    int16_t begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 1.6);

    /*!
      \brief Sets the module to standby mode (overload for PhysicalLayer compatibility, uses 13 MHz RC oscillator).
      \returns \ref status_codes
    */
    int16_t standby() override;

    /*!
      \brief Sets the module to standby mode.
      \param mode Oscillator to be used in standby mode. Can be set to RADIOLIB_LR2021_STANDBY_RC (13 MHz RC oscillator)
      or RADIOLIB_LR2021_STANDBY_XOSC (32 MHz external crystal oscillator).
      \param wakeup Whether to force the module to wake up. Setting to true will immediately attempt to wake up the module.
      \returns \ref status_codes
    */
    int16_t standby(uint8_t mode, bool wakeup = true);

    int16_t sleep();
    int16_t sleep(uint8_t cfg, uint32_t time);
    
    // configuration methods

    /*!
      \brief Sets LoRa bandwidth. Allowed values are 31.25, 41.67, 62.5, 83.34, 125.0, 
      101.56, 203.13, 250.0, 406.25, 500.0 kHz, 812.5 kHz and 1000.0 kHz.
      \param bw LoRa bandwidth to be set in kHz.
      \returns \ref status_codes
    */
    int16_t setBandwidth(float bw);

    /*!
      \brief Sets LoRa spreading factor. Allowed values range from 5 to 12.
      \param sf LoRa spreading factor to be set.
      \param legacy Enable legacy mode for SF6 - this allows to communicate with SX127x at SF6.
      \returns \ref status_codes
    */
    int16_t setSpreadingFactor(uint8_t sf, bool legacy = false);

    /*!
      \brief Sets LoRa coding rate denominator. Allowed values range from 4 to 8. Note that a value of 4 means no coding, 
      is undocumented and not recommended without your own FEC.
      \param cr LoRa coding rate denominator to be set.
      \param longInterleave Enable long interleaver when set to true.
      Note that with long interleaver enabled, CR 4/7 is not possible, there are packet length restrictions,
      and it is not compatible with SX127x radios!
      \returns \ref status_codes
    */
    int16_t setCodingRate(uint8_t cr, bool longInterleave = false);

    /*!
      \brief Sets LoRa sync word.
      \param syncWord LoRa sync word to be set.
      \returns \ref status_codes
    */
    int16_t setSyncWord(uint8_t syncWord);

    /*!
      \brief Sets preamble length for LoRa or GFSK modem. Allowed values range from 1 to 65535.
      \param preambleLength Preamble length to be set in symbols (LoRa) or bits (GFSK).
      \returns \ref status_codes
    */
    int16_t setPreambleLength(size_t preambleLength) override;

    /*!
      \brief Sets TCXO (Temperature Compensated Crystal Oscillator) configuration.
      \param voltage TCXO reference voltage in volts. Allowed values are 1.6, 1.7, 1.8, 2.2. 2.4, 2.7, 3.0 and 3.3 V.
      Set to 0 to disable TCXO.
      NOTE: After setting this parameter to 0, the module will be reset (since there's no other way to disable TCXO).
      \param delay TCXO timeout in us. Defaults to 5000 us.
      \returns \ref status_codes
    */
    int16_t setTCXO(float voltage, uint32_t delay = 5000);

    /*!
      \brief Sets CRC configuration.
      \param len CRC length in bytes, Allowed values are 1 or 2, set to 0 to disable CRC.
      \param initial Initial CRC value. GFSK only. Defaults to 0x1D0F (CCIT CRC).
      \param polynomial Polynomial for CRC calculation. GFSK only. Defaults to 0x1021 (CCIT CRC).
      \param inverted Invert CRC bytes. GFSK only. Defaults to true (CCIT CRC).
      \returns \ref status_codes
    */
    int16_t setCRC(uint8_t len, uint32_t initial = 0x00001D0FUL, uint32_t polynomial = 0x00001021UL, bool inverted = true);

    /*!
      \brief Enable/disable inversion of the I and Q signals
      \param enable IQ inversion enabled (true) or disabled (false);
      \returns \ref status_codes
    */
    int16_t invertIQ(bool enable) override;
    
#if !RADIOLIB_GODMODE && !RADIOLIB_LOW_LEVEL
  protected:
#endif
    Module* getMod() override;
    
#if !RADIOLIB_GODMODE
  protected:
#endif

#if !RADIOLIB_GODMODE
  private:
#endif

    bool findChip(void);
    int16_t config(uint8_t modem);

    // chip control commands
    int16_t readRadioRxFifo(uint8_t* data, size_t len);
    int16_t writeRadioTxFifo(uint8_t* data, size_t len);
    int16_t writeRegMem32(uint32_t addr, const uint32_t* data, size_t len);
    int16_t writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data);
    int16_t readRegMem32(uint32_t addr, uint32_t* data, size_t len);
    int16_t setFs(void);
    int16_t setAdditionalRegToRetain(uint8_t slot, uint32_t addr);
    int16_t setRx(uint32_t timeout);
    int16_t setTx(uint32_t timeout);
    int16_t setRxTxFallbackMode(uint8_t mode);
    int16_t setRxDutyCycle(uint32_t rxMaxTime, uint32_t cycleTime, uint8_t cfg);
    int16_t autoTxRx(uint32_t delay, uint8_t mode, uint32_t timeout);
    int16_t getRxPktLength(uint16_t* len);
    int16_t resetRxStats(void);
    int16_t setDefaultRxTxTimeout(uint32_t rxTimeout, uint32_t txTimeout);
    int16_t setRegMode(uint8_t simoUsage, uint8_t rampTimes[4]);
    int16_t calibrate(uint8_t blocks);
    int16_t calibrateFrontEnd(uint16_t freq[3]);
    int16_t getVbat(uint8_t resolution, uint16_t* vbat);
    int16_t getTemp(uint8_t source, uint8_t resolution, float* temp);
    int16_t setEolConfig(bool enable, uint8_t trim);
    int16_t getRandomNumber(uint32_t* rnd);
    int16_t getVersion(uint8_t* major, uint8_t* minor);
    int16_t clearErrors(void);
    int16_t getErrors(uint16_t* err);
    int16_t setDioFunction(uint8_t dio, uint8_t func, uint8_t pullDrive);
    int16_t setDioIrqConfig(uint8_t dio, uint32_t irq);
    int16_t clearIrq(uint32_t irq);
    int16_t getAndClearIrqStatus(uint32_t* irq);
    int16_t configFifoIrq(uint8_t rxFifoIrq, uint8_t txFifoIrq, uint8_t rxHighThreshold, uint8_t txHighThreshold);
    int16_t getFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags);
    int16_t clearFifoIrqFlags(uint8_t rxFifoFlags, uint8_t txFifoFlags);
    int16_t getAndClearFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags);
    int16_t getRxFifoLevel(uint16_t* level);
    int16_t getTxFifoLevel(uint16_t* level);
    int16_t clearRxFifo(void);
    int16_t clearTxFifo(void);
    int16_t configLfClock(uint8_t cfg);
    int16_t configClkOutputs(uint8_t scaling);
    int16_t setTcxoMode(uint8_t tune, uint32_t startTime);
    int16_t setXoscCpTrim(uint8_t xta, uint8_t xtb, uint8_t startTime);

    // radio frequency front end commands
    int16_t setRfFrequency(uint32_t rfFreq);
    int16_t setRxPath(uint8_t rxPath, uint8_t rxBoost);
    int16_t getRssiInst(float* rssi);
    int16_t setRssiCalibration(uint8_t rxPath, uint16_t gain[RADIOLIB_LR2021_GAIN_TABLE_LENGTH], uint8_t noiseFloor[RADIOLIB_LR2021_GAIN_TABLE_LENGTH]);
    int16_t setTimestampSource(uint8_t index, uint8_t source);
    int16_t getTimestampValue(uint8_t index, uint32_t* timestamp);
    int16_t setCca(uint32_t duration, uint8_t gain);
    int16_t getCcaResult(float* rssiMin, float* rssiMax, float* rssiAvg);
    int16_t setAgcGainManual(uint8_t gain);
    int16_t setCadParams(uint32_t cadTimeout, uint8_t threshold, uint8_t exitMode, uint32_t trxTimeout);
    int16_t setCad(void);
    int16_t selPa(uint8_t pa);
    int16_t setPaConfig(uint8_t pa, uint8_t paLfMode, uint8_t paLfDutyCycle, uint8_t paLfSlices, uint8_t paHfDutyCycle);
    int16_t setTxParams(int8_t txPower, uint8_t rampTime);

    // modem configuration commands
    int16_t setPacketType(uint8_t packetType);
    int16_t getPacketType(uint8_t* packetType);

    // LoRa commands
    int16_t setLoRaModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro);
    int16_t setLoRaPacketParams(uint16_t preambleLen, uint8_t hdrType, uint8_t payloadLen, uint8_t crcType, uint8_t invertIQ);
    int16_t setLoRaSynchTimeout(uint8_t numSymbols, bool format);
    int16_t setLoRaSyncword(uint8_t syncword);
    int16_t setLoRaSideDetConfig(uint8_t* configs, size_t numSideDets);
    int16_t setLoRaSideDetSyncword(uint8_t* syncwords, size_t numSideDets);
    int16_t setLoRaCadParams(uint8_t numSymbols, bool preambleOnly, uint8_t pnrDelta, uint8_t cadExitMode, uint32_t timeout, uint8_t detPeak);
    int16_t setLoRaCad(void);
    int16_t getLoRaRxStats(uint16_t* pktRxTotal, uint16_t* pktCrcError, uint16_t* headerCrcError, uint16_t* falseSynch);
    int16_t getLoRaPacketStatus(uint8_t* crc, uint8_t* cr, uint8_t* packetLen, float* snrPacket, float* rssiPacket, float* rssiSignalPacket);
    int16_t setLoRaAddress(uint8_t addrLen, uint8_t addrPos, uint8_t* addr);
    int16_t setLoRaHopping(uint8_t hopCtrl, uint16_t hopPeriod, uint32_t* freqHops, size_t numFreqHops);
    int16_t setLoRaTxSync(uint8_t function, uint8_t dioNum);
    int16_t setLoRaSideDetCad(uint8_t* pnrDelta, uint8_t* detPeak, size_t numSideDets);

    // ranging commands
    int16_t setRangingAddr(uint32_t addr, uint8_t checkLen);
    int16_t setRangingReqAddr(uint32_t addr);
    int16_t getRangingResult(uint8_t type, uint32_t* rng1, uint8_t* rssi1, uint32_t* rng2);
    int16_t getRangingStats(uint16_t* exchangeValid, uint16_t* requestValid, uint16_t* responseDone, uint16_t* timeout, uint16_t* requestDiscarded);
    int16_t setRangingTxRxDelay(uint32_t delay);
    int16_t setRangingParams(bool spyMode, uint8_t nbSymbols);

    // GFSK commands
    int16_t setFskModulationParams(uint32_t bitRate, uint8_t pulseShape, uint8_t rxBw, uint32_t freqDev);
    int16_t setFskPacketParams(uint16_t preambleLen, uint8_t preambleDetect, bool longPreamble, bool pldLenBits, uint8_t addrComp, uint8_t packetFormat, uint16_t payloadLen, uint8_t crc, uint8_t dcFree);
    int16_t setFskWhiteningParams(uint8_t whitenType, uint16_t init);
    int16_t setFskCrcParams(uint32_t poly, uint32_t init);
    int16_t setFskSyncword(uint8_t* syncWord, size_t syncWordLen, bool msbFirst);
    int16_t setFskAddress(uint8_t addrNode, uint8_t addrBroadcast);
    int16_t getFskRxStats(uint16_t* packetRx, uint16_t* packetCrcError, uint16_t* lenError, uint16_t* preambleDet, uint16_t* syncOk, uint16_t* syncFail, uint16_t* timeout);
    int16_t getFskPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, bool* addrMatchNode, bool* addrMatchBroadcast, float* lqi);

    // OQPSK commands
    int16_t setOqpskParams(uint8_t mode, uint8_t rxBw, uint8_t payloadLen, uint16_t preambleLen, bool addrFilt, bool fcsManual);
    int16_t getOqpskRxStats(uint16_t* packetRx, uint16_t* crcError, uint16_t* lenError);
    int16_t getOqpskPacketStatus(uint8_t* rxHeader, uint16_t* payloadLen, float* rssiAvg, float* rssiSync, float* lqi);
    int16_t setOqpskPacketLen(uint8_t len);
    int16_t setOqpskAddress(uint8_t longDestAddr[8], uint16_t shortDestAddr, uint16_t panId, uint8_t transId);

    // BPSK commands
    int16_t setBpskModulationParams(uint32_t bitRate, uint8_t pulseShape, bool diff, uint8_t diffInit);
    int16_t setBpskPacketParams(uint8_t payloadLen, uint8_t mode, bool sigFoxControlMsg, uint8_t sigFoxRank);

    // FLRC commands
    int16_t setFlrcModulationParams(uint8_t brBw, uint8_t cr, uint8_t pulseShape);
    int16_t setFlrcPacketParams(uint8_t agcPreambleLen, uint8_t syncWordLen, uint8_t syncWordTx, uint8_t syncMatch, bool fixedLength, uint8_t crc, uint16_t payloadLen);
    int16_t getFlrcRxStats(uint16_t* packetRx, uint16_t* packetCrcError, uint16_t* lenError);
    int16_t getFlrcPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, uint8_t* syncWordNum);
    int16_t setFlrcSyncWord(uint8_t syncWordNum, uint32_t syncWord);

    // LR-FHSS commands
    int16_t lrFhssSetSyncword(uint32_t syncWord);
    //! \TODO: [LR2021] Implement reading/writing LR-FHSS hopping table 
    //int16_t readLrFhssHoppingTable(LR2021LrFhssHopTableEntry_t* hopTable[40], size_t* hopTableLen);
    //int16_t writeLrFhssHoppingTable(LR2021LrFhssHopTableEntry_t* hopTable[40], size_t hopTableLen);

    // OOK commands
    int16_t setOokModulationParams(uint32_t bitRate, uint8_t pulseShape, uint8_t rxBw, uint8_t depth);
    int16_t setOokPacketParams(uint16_t preambleLen, uint8_t addrComp, uint8_t packetFormat, uint16_t payloadLen, uint8_t crc, uint8_t manchester);
    int16_t setOokCrcParams(uint32_t poly, uint32_t init);
    int16_t setOokSyncword(uint8_t* syncWord, size_t syncWordLen, bool msbFirst);
    int16_t setOokAddress(uint8_t addrNode, uint8_t addrBroadcast);
    int16_t getOokRxStats(uint16_t* packetRx, uint16_t* crcError, uint16_t* lenError);
    int16_t getOokPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, bool* addrMatchNode, bool* addrMatchBroadcast, float* lqi);
    int16_t setOokDetector(uint16_t preamblePattern, uint8_t patternLen, uint8_t patternNumRepeaters, bool syncWordRaw, bool sofDelimiterRising, uint8_t sofDelimiterLen);

    // test commands
    int16_t setTxTestMode(uint8_t mode);
};

#endif

#endif
