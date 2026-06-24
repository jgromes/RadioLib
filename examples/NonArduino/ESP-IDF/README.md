# RadioLib ESP-IDF example

This example shows how to use RadioLib as ESP-IDF component, without the Arduino framework. To run in ESP-IDF (or on any other platform), RadioLib includes an internal hardware abstraction layer (HAL). This software layer takes care of basic interaction with the hardware, such as performing SPI transaction or GPIO operations.

## FreeRTOS tick rate

The ESP-IDF HAL implements `delay(ms)` as `vTaskDelay(pdMS_TO_TICKS(ms))`, so its resolution is the FreeRTOS tick. ESP-IDF defaults `CONFIG_FREERTOS_HZ` to `100` (10 ms per tick), which rounds small delays down — e.g. `delay(5)` becomes `vTaskDelay(0)` and does not wait at all.

The Arduino ESP32 core works around this by forcing `CONFIG_FREERTOS_HZ=1000` (see the [arduino-esp32 CMakeLists.txt](https://github.com/espressif/arduino-esp32/blob/master/CMakeLists.txt)). This example does the same via `sdkconfig.defaults`. If you copy the HAL into your own ESP-IDF project, set `CONFIG_FREERTOS_HZ=1000` in your `sdkconfig` to get accurate 1 ms-resolution delays.

## Structure of the example

* `main/CMakeLists.txt` - IDF component CMake file
* `main/idf_component.yml` - declaration of the RadioLib dependency for this example
* `main/main.cpp` - the example source code
* `sdkconfig.defaults` - List of preset configuration option values for ESP-IDF. All other options use default values provided by ESP-IDF.
