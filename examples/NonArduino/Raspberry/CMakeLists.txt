cmake_minimum_required(VERSION 3.18)

# create the project
project(rpi-sx1261)

# when using debuggers such as gdb, the following line can be used
#set(CMAKE_BUILD_TYPE Debug)

# add the RadioLib source directory
# this is a bit of a hack because the example is being built within a library
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../RadioLib" "${CMAKE_CURRENT_BINARY_DIR}/RadioLib")

# add the executable
add_executable(${PROJECT_NAME} main.cpp)

# link both libraries
target_link_libraries(${PROJECT_NAME} RadioLib pigpio)

# you can also specify RadioLib compile-time flags here
#target_compile_definitions(${PROJECT_NAME} PUBLIC RADIOLIB_DEBUG RADIOLIB_VERBOSE)
