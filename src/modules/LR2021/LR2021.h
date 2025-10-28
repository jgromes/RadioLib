#if !defined(RADIOLIB_LR2021_H)
#define RADIOLIB_LR2021_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

#include "../../Module.h"

#include "../../protocols/PhysicalLayer/PhysicalLayer.h"
#include "../LR11x0/LR_common.h"
#include "LR2021_commands.h"

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
    explicit LR2021(Module* mod);
    
    int16_t sleep();
    int16_t sleep(uint8_t cfg, uint32_t time);
    
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
    Module* mod;

    // cached LoRa parameters
    bool ldroAuto = true;
    uint8_t bandwidth = 0, spreadingFactor = 0, ldrOptimize = 0;
    float bandwidthKhz = 0;

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
    int16_t setLoRaSyncword(uint16_t syncword);
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

};

#endif

#endif
