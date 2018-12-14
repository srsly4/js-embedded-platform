#!/usr/bin/env bash

touch ~/getenv.sh
source ~/getenv.sh

cmake . -DPLATFORM="$TARGET_PLATFORM"
make