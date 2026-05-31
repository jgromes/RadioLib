#!/bin/bash

set -e

args=$1

# use clang
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

# build the test binary
rm -rf build
mkdir -p build
mkdir -p out
cd build
cmake -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} ${args}" ..
make -j4

# run it for 2^17 iterations
cd ..
./build/radiolib-fuzztest -runs=131072 -artifact_prefix=out/
