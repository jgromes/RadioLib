#!/bin/bash

#board=arduino:avr:mega
board="$1"
#skip="(STM32WL|LR11x0_Firmware_Update|NonArduino)"
skip="$2"
#options=""
options="$3"

# file for saving the compiled binary size reports
size_file="size_$board.txt"
rm -f $size_file

path="../../../examples"
for example in $(find $path -name '*.ino' | sort); do
  # check whether to skip this sketch
  if [ ! -z '$skip' ] && [[ ${example} =~ ${skip} ]]; then
    # skip sketch
    echo -e "\n\033[1;33mSkipped ${example##*/} (matched with $skip)\033[0m";
  else
    # apply special flags for LoRaWAN
    if [[ ${example} =~ "LoRaWAN" ]]; then
      flags="-DRADIOLIB_LORAWAN_DEV_ADDR=0 -DRADIOLIB_LORAWAN_FNWKSINT_KEY=0 \
      -DRADIOLIB_LORAWAN_SNWKSINT_KEY=0 -DRADIOLIB_LORAWAN_NWKSENC_KEY=0 \
      -DRADIOLIB_LORAWAN_APPS_KEY=0 -DRADIOLIB_LORAWAN_APP_KEY=0 \
      -DRADIOLIB_LORAWAN_NWK_KEY=0 -DRADIOLIB_LORAWAN_DEV_EUI=0 \
      -DRADIOLIB_LORAWAN_MC_DEV_ADDR=0 -DRADIOLIB_LORAWAN_MC_APP_SKEY=0 \
      -DRADIOLIB_LORAWAN_MC_NWK_SKEY=0 -DARDUINO_TTGO_LORA32_V1"
    fi

    # build sketch
    echo -e "\n\033[1;33mBuilding ${example##*/} ... \033[0m";
    board_opts=$board$options
    ./build_arduino.sh $board_opts $example "$flags"
    if [ $? -ne 0 ]; then
      echo -e "\033[1;31m${example##*/} build FAILED\033[0m\n";
      exit 1;
    else
      echo -e "\033[1;32m${example##*/} build PASSED\033[0m\n";
      dir="$(dirname -- "$example")"
      file="$(basename -- "$example")"
      size="$(size $dir/build/*/$file.elf)"
      echo $size >> $size_file
    fi
  fi
done
