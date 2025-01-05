#!/bin/bash

set -e

# build the test binary
mkdir -p build
cd build
cmake -G "CodeBlocks - Unix Makefiles" ..
make -j4

# run it
cd ..
./build/radiolib-unittest --log_level=message
