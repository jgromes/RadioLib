#!/bin/bash

set -e

# build the test binary
rm -rf build
mkdir build
cd build
cmake -G "CodeBlocks - Unix Makefiles" ..
make -j4

# run it
cd ..

# TODO silly workaround to prevent this from getting stuck in CI
./build/radiolib-unittest --log_level=message --detect_memory_leaks --run_test=suite_Module
./build/radiolib-unittest --log_level=message --detect_memory_leaks --run_test=suite_TimeOnAir
./build/radiolib-unittest --log_level=message --detect_memory_leaks --run_test=suite_PhyComplete
./build/radiolib-unittest --log_level=message --detect_memory_leaks --run_test=suite_Crypto
