# js-embedded-platform
Managed Javascript interpreter for embedded platforms

## Compiling

To compile platform for designed platform, configure CMake project with (in project's root directory):
```
cmake -DPLATFORM==<platform>
```

And compile with:
```
make
```

Programmable binaries can be uploaded onto the board with:
```
make UPLOAD
```

## Supported platforms
Currently there are two supported platforms:
* STM32F429ZI Nucleo-144 - with GPIO support (platform key is `nucleo-f429zi`). `arm-none-eabi-gcc` toolchain is required along with `openocd`
* Linux - currently only for testing purposes (platform key is `unix`)
