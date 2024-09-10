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
arduino-cli compile --fqbn Seeeduino:nrf52:wio_tracker_1110 --quiet --output-dir ./build

echo >/tmp/adanrf
while ! grep -q 'Device programmed' /tmp/adanrf; do
    stty -F $PORT 1200
    sleep 1
    while ! [ -c $PORT ]; do echo "waiting for bootloder"; sleep 1; done
    sleep 1
    adafruit-nrfutil dfu serial -pkg build/$NAME.ino.zip -p $PORT -b 115200 -sb >/tmp/adanrf
    head -2 /tmp/adanrf
    sleep 1
done
sleep 2
while ! [ -c $PORT ]; do echo "waiting for console"; sleep 1; done
sleep 1
# while ! [[ -e $(echo $PORT) ]]; do echo ls /dev/ttyACM0 $PORT; sleep 1; done
arduino-cli monitor --fqbn Seeeduino:nrf52:wio_tracker_1110 -p $PORT -c 115200
