#!/usr/bin/env bash

git clone git://github.com/raspberrypi/tools.git RPi-tools
cd RPi-tools

git checkout 5caa7046982f0539cf5380f94da04b31129ed521

RASPBERRY_SYSROOT=`pwd`/arm-bcm2708/arm-linux-gnueabihf/arm-linux-gnueabihf/sysroot
RASPBERRY_COMPILERS=`pwd`/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin

echo "export RASPBERRY_SYSROOT=$RASPBERRY_SYSROOT" > ~/getenv.sh
echo "export RASPBERRY_COMPILERS=$RASPBERRY_COMPILERS" >> ~/getenv.sh