#if !defined(_RADIOLIB_MODULE_H)
#define _RADIOLIB_MODULE_H

#include "TypeDef.h"

#if defined(RADIOLIB_BUILD_ARDUINO)
  #include <SPI.h>
#endif

/*!
  \class Module

  \brief Implements all common low-level methods to control the wireless module.
  Every module class contains one private instance of this class.
*/
class Module {
  public:
    /*!
     * \brief The maximum number of pins supported by the RF switch
     * code.
     *
     * Note: It is not recommended to use this constant in your sketch
     * when defining a rfswitch pins array, to prevent issues when this
     * value is ever increased and such an array gets extra zero
     * elements (that will be interpreted as pin 0).
     */
    static const size_t RFSWITCH_MAX_PINS = 3;

    /*!
     * Description of RF switch pin states for a single mode.
     *
     * See setRfSwitchTable() for details.
     */
    struct RfSwitchMode_t {
      uint8_t mode;
      RADIOLIB_PIN_STATUS values[RFSWITCH_MAX_PINS];
    };

    /*!
     * Constants to use in a mode table set be setRfSwitchTable. These
     * constants work for most radios, but some radios define their own
     * constants to be used instead.
     *
     * See setRfSwitchTable() for details.
     */
    enum OpMode_t {
      /*! End of table marker, use \ref END_OF_MODE_TABLE constant
       * instead. Value is zero to ensure zero-initialized mode ends the
       * table */
      MODE_END_OF_TABLE = 0,
      /*! Idle mode */
      MODE_IDLE,
      /*! Receive mode */
      MODE_RX,
      /*! Transmission mode */
      MODE_TX,
    };

    /*!
     * Value to use as the last element in a mode table to indicate the
     * end of the table.
     *
     * See setRfSwitchTable() for details.
     */
    static constexpr RfSwitchMode_t END_OF_MODE_TABLE = {MODE_END_OF_TABLE, {}};

    #if defined(RADIOLIB_BUILD_ARDUINO)

    /*!
      \brief Arduino Module constructor. Will use the default SPI interface and automatically initialize it

      \param cs Arduino pin to be used as chip select.

      \param irq Arduino pin to be used as interrupt/GPIO.

      \param rst Arduino pin to be used as hardware reset for the module.

      \param gpio Arduino pin to be used as additional interrupt/GPIO.
    */
    Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio = RADIOLIB_NC);

    /*!
      \brief Arduino Module constructor. Will not attempt SPI interface initialization.

      \param cs Arduino pin to be used as chip select.

      \param irq Arduino pin to be used as interrupt/GPIO.

      \param rst Arduino pin to be used as hardware reset for the module.

      \param gpio Arduino pin to be used as additional interrupt/GPIO.

      \param spi SPI interface to be used, can also use software SPI implementations.

      \param spiSettings SPI interface settings.
    */
    Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio, SPIClass& spi, SPISettings spiSettings = RADIOLIB_DEFAULT_SPI_SETTINGS);

    #else

    /*!
      \brief Default constructor.

      \param cs Pin to be used as chip select.

      \param irq Pin to be used as interrupt/GPIO.

      \param rst Pin to be used as hardware reset for the module.

      \param gpio Pin to be used as additional interrupt/GPIO.
    */
    Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio = RADIOLIB_NC);

    #endif

    /*!
      \brief Copy constructor.

      \param mod Module instance to copy.
    */
    Module(const Module& mod);

    /*!
      \brief Overload for assignment operator.

      \param frame rvalue Module.
    */
    Module& operator=(const Module& mod);

    // public member variables

    /*!
      \brief Basic SPI read command. Defaults to 0x00.
    */
    uint8_t SPIreadCommand = 0b00000000;

    /*!
      \brief Basic SPI write command. Defaults to 0x80.
    */
    uint8_t SPIwriteCommand = 0b10000000;

    #if defined(RADIOLIB_INTERRUPT_TIMING)

    /*!
      \brief Timer interrupt setup callback typedef.
    */
    typedef void (*TimerSetupCb_t)(uint32_t len);

    /*!
      \brief Callback to timer interrupt setup function when running in interrupt timing control mode.
    */
    TimerSetupCb_t TimerSetupCb = nullptr;

    /*!
      \brief Timer flag variable to be controlled by a platform-dependent interrupt.
    */
    volatile bool TimerFlag = false;

    #endif

    // basic methods

    /*!
      \brief Initialize low-level module control.
    */
    void init();

    /*!
      \brief Terminate low-level module control.
    */
    void term();

    // SPI methods

    /*!
      \brief SPI read method that automatically masks unused bits. This method is the preferred SPI read mechanism.

      \param reg Address of SPI register to read.

      \param msb Most significant bit of the register variable. Bits above this one will be masked out.

      \param lsb Least significant bit of the register variable. Bits below this one will be masked out.

      \returns Masked register value or status code.
    */
    int16_t SPIgetRegValue(uint8_t reg, uint8_t msb = 7, uint8_t lsb = 0);

    /*!
      \brief Overwrite-safe SPI write method with verification. This method is the preferred SPI write mechanism.

      \param reg Address of SPI register to write.

      \param value Single byte value that will be written to the SPI register.

      \param msb Most significant bit of the register variable. Bits above this one will not be affected by the write operation.

      \param lsb Least significant bit of the register variable. Bits below this one will not be affected by the write operation.

      \param checkInterval Number of milliseconds between register writing and verification reading. Some registers need up to 10ms to process the change.

      \param checkMask Mask of bits to check, only bits set to 1 will be verified.

      \returns \ref status_codes
    */
    int16_t SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb = 7, uint8_t lsb = 0, uint8_t checkInterval = 2, uint8_t checkMask = 0xFF);

    /*!
      \brief SPI burst read method.

      \param reg Address of SPI register to read.

      \param numBytes Number of bytes that will be read.

      \param inBytes Pointer to array that will hold the read data.
    */
    void SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes);

    /*!
      \brief SPI basic read method. Use of this method is reserved for special cases, SPIgetRegValue should be used instead.

      \param reg Address of SPI register to read.

      \returns Value that was read from register.
    */
    uint8_t SPIreadRegister(uint8_t reg);

    /*!
      \brief SPI burst write method.

      \param reg Address of SPI register to write.

      \param data Pointer to array that holds the data that will be written.

      \param numBytes Number of bytes that will be written.
    */
    void SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, uint8_t numBytes);

    /*!
      \brief SPI basic write method. Use of this method is reserved for special cases, SPIsetRegValue should be used instead.

      \param reg Address of SPI register to write.

      \param data Value that will be written to the register.
    */
    void SPIwriteRegister(uint8_t reg, uint8_t data);

    /*!
      \brief SPI single transfer method.

      \param cmd SPI access command (read/write/burst/...).

      \param reg Address of SPI register to transfer to/from.

      \param dataOut Data that will be transfered from master to slave.

      \param dataIn Data that was transfered from slave to master.

      \param numBytes Number of bytes to transfer.
    */
    void SPItransfer(uint8_t cmd, uint8_t reg, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes);

    // pin number access methods

    /*!
      \brief Access method to get the pin number of SPI chip select.

      \returns Pin number of SPI chip select configured in the constructor.
    */
    RADIOLIB_PIN_TYPE getCs() const { return(_cs); }

    /*!
      \brief Access method to get the pin number of interrupt/GPIO.

      \returns Pin number of interrupt/GPIO configured in the constructor.
    */
    RADIOLIB_PIN_TYPE getIrq() const { return(_irq); }

    /*!
      \brief Access method to get the pin number of hardware reset pin.

      \returns Pin number of hardware reset pin configured in the constructor.
    */
    RADIOLIB_PIN_TYPE getRst() const { return(_rst); }

    /*!
      \brief Access method to get the pin number of second interrupt/GPIO.

      \returns Pin number of second interrupt/GPIO configured in the constructor.
    */
    RADIOLIB_PIN_TYPE getGpio() const { return(_gpio); }

    /*!
      \brief Some modules contain external RF switch controlled by pins.
      This function gives RadioLib control over those pins to
      automatically switch between various modes: When idle both pins
      will be LOW, during TX the `txEn` pin will be HIGH, during RX the
      `rxPin` will be HIGH.

      Radiolib will automatically set the pin mode and value of these
      pins, so do not control them from the sketch.

      When more than two pins or more control over the output values are
      needed, use the setRfSwitchTable() function.

      \param rxEn RX enable pin.
      \param txEn TX enable pin.
    */
    void setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn);

    /*!
      \brief Some modules contain external RF switch controlled by pins.
      This function gives RadioLib control over those pins to
      automatically switch between various modes.

      Radiolib will automatically set the pin mode and value of these
      pins, so do not control them from the sketch.


      \param pins A reference to an array of pins to control. This
      should always be an array of 3 elements. If you need less pins,
      use RADIOLIB_NC for the unused elements.

      \param table A reference to an array of pin values to use for each
      supported mode. Each element is an RfSwitchMode_T struct that
      lists the mode for which it applies and the values for each of the
      pins passed in the pins argument respectively.

      The `pins` array will be copied into the Module object, so the
      original array can be deallocated after this call. However,
      a reference to the `table` array will be stored, so that array
      must remain valid as long RadioLib is being used.

      The `mode` field in each table row should normally use any of the
      `MODE_*` constants from the Module::OpMode_t enum. However, some
      radios support additional modes and will define their own OpMode_t
      enum.

      The length of the table is variable (to support radios that add
      additional modes), so the table must always be terminated with the
      special END_OF_MODE_TABLE value.

      Normally all modes should be listed in the table, but for some
      radios, modes can be omitted to indicate they are not supported
      (e.g. when a radio has a high power and low power TX mode but
      external circuitry only supports low power). If applicable, this
      is documented in the radio class itself.

      #### Example
      For example, on a board that has an RF switch with an enable pin
      connected to PA0 and a TX/RX select pin connected to PA1:

      \code
      // In global scope, define the pin array and mode table
      static const RADIOLIB_PIN_TYPE rfswitch_pins[] =
                             {PA0,  PA1,  RADIOLIB_NC};
      static const Module::RfSwitchMode_t rfswitch_table[] = {
        {Module::MODE_IDLE,  {LOW,  LOW}},
        {Module::MODE_RX,    {HIGH, LOW}},
        {Module::MODE_TX,    {HIGH, HIGH}},
         Module::END_OF_MODE_TABLE,
      };

      void setup() {
        ...
        // Then somewhere in setup, pass them to radiolib
        radio.setRfSwitchTable(rfswitch_pins, rfswitch_table);
        ...
      }
      \endcode
    */

    void setRfSwitchTable(const RADIOLIB_PIN_TYPE (&pins)[RFSWITCH_MAX_PINS], const RfSwitchMode_t table[]);

    /*!
     * \brief Find a mode in the RfSwitchTable.
     *
     * \param The mode to find.
     *
     * \returns A pointer to the RfSwitchMode_t struct in the table that
     * matches the passed mode. Returns nullptr if no rfswitch pins are
     * configured, or the passed mode is not listed in the table.
     */
    const RfSwitchMode_t *findRfSwitchMode(uint8_t mode) const;

    /*!
      \brief Set RF switch state.
      \param mode The mode to set. This must be one of the MODE_ constants, or a radio-specific constant.
    */
    void setRfSwitchState(uint8_t mode);

    /*!
      \brief Wait for time to elapse, either using the microsecond timer, or the TimerFlag.
      Note that in interrupt timing mode, it is up to the user to set up the timing interrupt!

      \param start Waiting start timestamp, in microseconds.

      \param len Waiting duration, in microseconds;
    */
    void waitForMicroseconds(uint32_t start, uint32_t len);

    // Arduino core overrides

    /*!
      \brief Arduino core pinMode override that checks RADIOLIB_NC as alias for unused pin.

      \param pin Pin to change the mode of.

      \param mode Which mode to set.
    */
    void pinMode(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_MODE mode);

    /*!
      \brief Arduino core digitalWrite override that checks RADIOLIB_NC as alias for unused pin.

      \param pin Pin to write to.

      \param value Whether to set the pin high or low.
    */
    void digitalWrite(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_STATUS value);

    /*!
      \brief Arduino core digitalWrite override that checks RADIOLIB_NC as alias for unused pin.

      \param pin Pin to read from.

      \returns Pin value.
    */
    RADIOLIB_PIN_STATUS digitalRead(RADIOLIB_PIN_TYPE pin);

    /*!
      \brief Arduino core tone override that checks RADIOLIB_NC as alias for unused pin and RADIOLIB_TONE_UNSUPPORTED to make sure the platform does support tone.

      \param pin Pin to write to.

      \param value Frequency to output.
    */
    void tone(RADIOLIB_PIN_TYPE pin, uint16_t value, uint32_t duration = 0);

    /*!
      \brief Arduino core noTone override that checks RADIOLIB_NC as alias for unused pin and RADIOLIB_TONE_UNSUPPORTED to make sure the platform does support tone.

      \param pin Pin to write to.
    */
    void noTone(RADIOLIB_PIN_TYPE pin);

    /*!
      \brief Arduino core attachInterrupt override.

      \param interruptNum Interrupt number.

      \param userFunc Interrupt service routine.

      \param mode Pin hcange direction.
    */
    void attachInterrupt(RADIOLIB_PIN_TYPE interruptNum, void (*userFunc)(void), RADIOLIB_INTERRUPT_STATUS mode);

    /*!
      \brief Arduino core detachInterrupt override.

      \param interruptNum Interrupt number.
    */
    void detachInterrupt(RADIOLIB_PIN_TYPE interruptNum);

    /*!
      \brief Arduino core yield override.
    */
    void yield();

    /*!
      \brief Arduino core delay override.

      \param ms Delay length in milliseconds.
    */
    void delay(uint32_t ms);

    /*!
      \brief Arduino core delayMicroseconds override.

      \param us Delay length in microseconds.
    */
    void delayMicroseconds(uint32_t us);

    /*!
      \brief Arduino core millis override.
    */
    uint32_t millis();

    /*!
      \brief Arduino core micros override.
    */
    uint32_t micros();

    /*!
      \brief Arduino core pulseIn override.
    */
    uint32_t pulseIn(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_STATUS state, uint32_t timeout);

    /*!
      \brief Arduino core SPI begin override.
    */
    void begin();

    /*!
      \brief Arduino core SPI beginTransaction override.
    */
    void beginTransaction();

    /*!
      \brief Arduino core SPI transfer override.
    */
    uint8_t transfer(uint8_t b);

    /*!
      \brief Arduino core SPI endTransaction override.
    */
    void endTransaction();

    /*!
      \brief Arduino core SPI end override.
    */
    void end();

    // helper functions to set up SPI overrides on Arduino
    #if defined(RADIOLIB_BUILD_ARDUINO)
    void SPIbegin();
    void SPIend();
    #endif
    virtual void SPIbeginTransaction();
    virtual uint8_t SPItransfer(uint8_t b);
    virtual void SPIendTransaction();

    /*!
      \brief Function to reflect bits within a byte.
    */
    static uint8_t flipBits(uint8_t b);

    /*!
      \brief Function to reflect bits within an integer.
    */
    static uint16_t flipBits16(uint16_t i);

    /*!
      \brief Function to dump data as hex into the debug port.

      \param data Data to dump.

      \param len Number of bytes to dump.
    */
    static void hexdump(uint8_t* data, size_t len);

    /*!
      \brief Function to dump device registers as hex into the debug port.

      \param start First address to dump.

      \param len Number of bytes to dump.
    */
    void regdump(uint8_t start, uint8_t len);

#if !defined(RADIOLIB_GODMODE)
  private:
#endif

    // pins
    RADIOLIB_PIN_TYPE _cs = RADIOLIB_NC;
    RADIOLIB_PIN_TYPE _irq = RADIOLIB_NC;
    RADIOLIB_PIN_TYPE _rst = RADIOLIB_NC;
    RADIOLIB_PIN_TYPE _gpio = RADIOLIB_NC;

    // SPI interface (Arduino only)
    #if defined(RADIOLIB_BUILD_ARDUINO)
    SPIClass* _spi = NULL;
    SPISettings _spiSettings = RADIOLIB_DEFAULT_SPI_SETTINGS;
    bool _initInterface = false;
    #endif

    // RF switch pins and table
    RADIOLIB_PIN_TYPE _rfSwitchPins[RFSWITCH_MAX_PINS] = { RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC };
    const RfSwitchMode_t *_rfSwitchTable = nullptr;

    #if defined(RADIOLIB_INTERRUPT_TIMING)
    uint32_t _prevTimingLen = 0;
    #endif

    // hardware abstraction layer callbacks
    // this is placed at the end of Module class because the callback generator macros
    // screw with the private/public access specifiers
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_PIN_MODE);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DIGITAL_WRITE);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DIGITAL_READ);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_TONE);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_NO_TONE);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_ATTACH_INTERRUPT);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DETACH_INTERRUPT);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_YIELD);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DELAY);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_DELAY_MICROSECONDS);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_MILLIS);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_MICROS);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_PULSE_IN);

    #if defined(RADIOLIB_BUILD_ARDUINO)
    RADIOLIB_GENERATE_CALLBACK_SPI(RADIOLIB_CB_ARGS_SPI_BEGIN);
    RADIOLIB_GENERATE_CALLBACK_SPI(RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION);
    RADIOLIB_GENERATE_CALLBACK_SPI(RADIOLIB_CB_ARGS_SPI_TRANSFER);
    RADIOLIB_GENERATE_CALLBACK_SPI(RADIOLIB_CB_ARGS_SPI_END_TRANSACTION);
    RADIOLIB_GENERATE_CALLBACK_SPI(RADIOLIB_CB_ARGS_SPI_END);
    #else
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_BEGIN);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_BEGIN_TRANSACTION);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_TRANSFER);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_END_TRANSACTION);
    RADIOLIB_GENERATE_CALLBACK(RADIOLIB_CB_ARGS_SPI_END);
    #endif
};

#endif
