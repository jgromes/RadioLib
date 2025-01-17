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
./build/radiolib-unittest --log_level=message
