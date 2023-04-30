cmake_minimum_required(VERSION 3.13)

project(radiolib)

file(GLOB_RECURSE RADIOLIB_SOURCES
  "src/*.cpp"
)

add_library(RadioLib ${RADIOLIB_SOURCES})

target_include_directories(RadioLib PUBLIC "src" "${CMAKE_CURRENT_BINARY_DIR}/include")
