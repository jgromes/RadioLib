# RadioLib Non-Arduino Tock Library CMake script
#
# Licensed under the MIT License
#
# Copyright (c) 2023 Alistair Francis <alistair@alistair23.me>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.18)

# create the project
project(tock-sx1261)

set(LINKER_SCRIPT $ENV{LIBTOCK_C_DIRECTORY}/userland_generic.ld)

if (RISCV_BUILD)
    include("tock-riscv.cmake")
else()
    include("tock-arm.cmake")
endif()

# when using debuggers such as gdb, the following line can be used
#set(CMAKE_BUILD_TYPE Debug)

# if you did not build RadioLib as shared library (see wiki),
# you will have to add it as source directory
# the following is just an example, yours will likely be different
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../RadioLib" "${CMAKE_CURRENT_BINARY_DIR}/RadioLib")

# add the executable
add_executable(${PROJECT_NAME} main.cpp)

# link with RadioLib and libtock-c
# libtock-c downloads prebuilt newlib/libstdc++/libgcc archives (built with
# the Tock PIC model). The versions are sometimes updated, let's try
# to find the latest version
file(GLOB TOCK_NEWLIB_DIRS LIST_DIRECTORIES true
    "$ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-newlib-*")
list(FILTER TOCK_NEWLIB_DIRS INCLUDE REGEX "[0-9]$")
list(SORT TOCK_NEWLIB_DIRS)
list(POP_BACK TOCK_NEWLIB_DIRS TOCK_NEWLIB_DIR)

file(GLOB TOCK_LIBCXX_DIRS LIST_DIRECTORIES true
    "$ENV{LIBTOCK_C_DIRECTORY}/lib/libtock-libc++-*")
list(FILTER TOCK_LIBCXX_DIRS INCLUDE REGEX "[0-9]$")
list(SORT TOCK_LIBCXX_DIRS)
list(POP_BACK TOCK_LIBCXX_DIRS TOCK_LIBCXX_DIR)

if (NOT TOCK_NEWLIB_DIR OR NOT TOCK_LIBCXX_DIR)
    message(FATAL_ERROR
        "No prebuilt libraries found in $ENV{LIBTOCK_C_DIRECTORY}/lib; "
        "build a libtock-c example (e.g. via build.sh) first to fetch them")
endif()

string(REGEX REPLACE ".*/libtock-libc\\+\\+-" "" TOCK_LIBCXX_GCC_VERSION "${TOCK_LIBCXX_DIR}")
message("libtock-c prebuilt libraries: ${TOCK_NEWLIB_DIR}, ${TOCK_LIBCXX_DIR}")

if (RISCV_BUILD)
    target_include_directories(RadioLib AFTER PUBLIC
        ${TOCK_NEWLIB_DIR}/riscv/riscv64-unknown-elf/include/
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC
        RadioLib
        -Wl,--start-group
        $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/rv32imc/libtock.a
        $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/rv32imc/libtocksync.a
        ${TOCK_LIBCXX_DIR}/riscv/lib/gcc/riscv64-unknown-elf/${TOCK_LIBCXX_GCC_VERSION}/rv32i/ilp32/libgcc.a
        ${TOCK_LIBCXX_DIR}/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libstdc++.a
        ${TOCK_NEWLIB_DIR}/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libc.a
        ${TOCK_NEWLIB_DIR}/riscv/riscv64-unknown-elf/lib/rv32i/ilp32/libm.a
        -Wl,--end-group
    )
else()
    target_link_options(${PROJECT_NAME} PUBLIC
        -B${TOCK_NEWLIB_DIR}/arm/arm-none-eabi/lib/
        -B${TOCK_LIBCXX_DIR}/arm/arm-none-eabi/lib/
        -B${TOCK_LIBCXX_DIR}/arm/lib/gcc/arm-none-eabi/${TOCK_LIBCXX_GCC_VERSION}/
    )

    target_link_libraries(${PROJECT_NAME} PUBLIC
        RadioLib
        -Wl,--start-group
        $ENV{LIBTOCK_C_DIRECTORY}/libtock/build/cortex-m4/libtock.a
        $ENV{LIBTOCK_C_DIRECTORY}/libtock-sync/build/cortex-m4/libtocksync.a
        -lstdc++ -lc -lm -lgcc
        -Wl,--end-group
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/
    $ENV{LIBTOCK_C_DIRECTORY}
)

# you can also specify RadioLib compile-time flags here
#target_compile_definitions(${PROJECT_NAME} PUBLIC RADIOLIB_DEBUG RADIOLIB_VERBOSE)
