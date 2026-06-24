cmake_minimum_required(VERSION 3.13)

# first gather the files since this is needed for both ESP-IDF as well as other builds
file(GLOB_RECURSE RADIOLIB_SOURCES
  "src/*.cpp"
)

# exclude all HAL source files
list(FILTER RADIOLIB_SOURCES EXCLUDE REGEX "src/hal/.*\\.cpp")

if(ESP_PLATFORM)
  # Build RadioLib as an ESP-IDF component
  # required because ESP-IDF runs cmake in script mode
  # and needs idf_component_register()

  # the ESP-IDF HAL was excluded from RADIOLIB_SOURCES by the filter above;
  # pass it directly to idf_component_register so RADIOLIB_SOURCES stays
  # HAL-free, the same way other build environments handle it
  file(GLOB RADIOLIB_ESP_IDF_HAL_SOURCES "src/hal/ESP-IDF/*.cpp")

  idf_component_register(
    SRCS ${RADIOLIB_SOURCES} ${RADIOLIB_ESP_IDF_HAL_SOURCES}
    INCLUDE_DIRS . src
    REQUIRES esp_driver_gpio esp_driver_spi esp_driver_ledc esp_timer
  )

  return()
endif()

if(CMAKE_SCRIPT_MODE_FILE)
  message(FATAL_ERROR "Attempted to build RadioLib in script mode")
endif()

project(radiolib)

add_library(RadioLib ${RADIOLIB_SOURCES})

target_include_directories(RadioLib
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# use c++20 standard
set_property(TARGET RadioLib PROPERTY CXX_STANDARD 20)

# enable most warnings
target_compile_options(RadioLib PRIVATE -Wall -Wextra -Wpedantic -Wdouble-promotion)

include(GNUInstallDirs)

install(TARGETS RadioLib
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/RadioLib
  FILES_MATCHING PATTERN "*.h"
)
