#! /bin/bash -e
# Build the firsmware from scratch, prereq: arduino-cli installed
# Lots of stuff copied from https://github.com/adafruit/ci-arduino/blob/master/install.sh

export ARDUINO_BOARD_MANAGER_ADDITIONAL_URLS=https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
set nonomatch=1

if ! which arduino-cli >/dev/null; then echo "Please install arduino-cli"; exit 1; fi

if ! [[ -d $HOME/.arduino15/packages/adafruit ]]; then
    arduino-cli core install Seeeduino:nrf52
fi

if ! [[ -f $HOME/.arduino15/library_index.json ]]; then
	arduino-cli lib update-index
fi
# arduino-cli lib install RadioLib@6.6.0 CRC@1.0.3 "Arduino Low Power@1.2.2"

PORT=$(echo /dev/serial/by-id/*Wio_Tracker*)
NAME=$(basename $PWD)
stty -F $PORT 1200; sleep 0.5
arduino-cli compile --fqbn Seeeduino:nrf52:wio_tracker_1110 --quiet --output-dir ./build

while true; do
    DEV=$(lsusb | grep 'Wio Tracker' | cut '-d ' -f4)
    echo -n "'Touching' port (dev=$DEV) ..."
    stty -F $PORT 1200
    # sleep 1
    # while true; do
    #     DEV2=$(lsusb | grep 'Wio Tracker' | cut '-d ' -f4)
    #     if [[ -n "$DEV2" ]] && [[ "$DEV2" != "$DEV" ]]; then echo -n " (dev=$DEV2) ... "; break; fi
    #     sleep 0.2
    # done
    # sleep 3
    while ! [ -c $PORT ]; do echo "waiting for bootloder"; sleep 1; done
    # sleep 1
    # lsusb | grep Seeed
    echo "Starting download"
    adafruit-nrfutil dfu serial -pkg build/$NAME.ino.zip -p $PORT -b 115200 -sb >/tmp/adanrf
    head -2 /tmp/adanrf
    grep -q 'Device programmed' /tmp/adanrf && break
    sleep 5
done
sleep 3
arduino-cli monitor --fqbn Seeeduino:nrf52:wio_tracker_1110 -p $PORT -c 115200
