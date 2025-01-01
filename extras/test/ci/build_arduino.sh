#!/bin/bash

board=$1
sketch=$2
flags=$3
warnings="all"

arduino-cli compile \
  --libraries ../../../../ \
  --fqbn $board \
  --build-property compiler.cpp.extra_flags="$flags" \
  --warnings=$warnings \
  $sketch --export-binaries
