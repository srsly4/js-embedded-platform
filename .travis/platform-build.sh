#!/usr/bin/env bash

wget https://cmake.org/files/v3.12/cmake-3.12.3-Linux-x86_64.sh -O=cmake.sh
chmod +x cmake.sh

./cmake.sh . -DPLATFORM="$TARGET_PLATFORM"
make