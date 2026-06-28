# RadioLib ESP-IDF example

This example shows how to use RadioLib as ESP-IDF component, without the Arduino framework. To run in ESP-IDF (or on any other platform), RadioLib includes an internal hardware abstraction layer (HAL). This software layer takes care of basic interaction with the hardware, such as performing SPI transaction or GPIO operations.

## FreeRTOS tick rate

The ESP-IDF HAL's `delay()` resolution depends on the FreeRTOS tick rate. The HAL refuses to build unless `CONFIG_FREERTOS_HZ=1000` so that radio timings — e.g. LoRaWAN RX1/RX2 windows that fire 1-2 ms after a transmit — are accurate. This example sets it via `sdkconfig.defaults`; do the same in any project that uses the HAL.

If you need to keep a lower tick rate, build with `-DRADIOLIB_RELAX_RTOS_TICK=1` to bypass the check. `delay()` will still be correct in that mode — it sleeps for the bulk of the wait and busy-waits the sub-tick remainder with `esp_rom_delay_us()` — but the busy-wait portion can be up to one tick period (10 ms at the IDF default of 100 Hz), which wastes CPU and power.

## Structure of the example

* `main/CMakeLists.txt` - IDF component CMake file
* `main/idf_component.yml` - declaration of the RadioLib dependency for this example
* `main/main.cpp` - the example source code
* `sdkconfig.defaults` - List of preset configuration option values for ESP-IDF. All other options use default values provided by ESP-IDF.
