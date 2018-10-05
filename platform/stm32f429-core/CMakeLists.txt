# Cross-compiler settings
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

add_definitions(-D__weak=__attribute__\(\(weak\)\) -DMBEDTLS_CONFIG_FILE=\"mbedtls_config.h\" -D__packed=__attribute__\(\(__packed__\)\) -DUSE_HAL_DRIVER -DSTM32F407xx )

set(FPU_FLAGS "-mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(COMMON_FLAGS "-mcpu=cortex-m4 ${FPU_FLAGS} -mthumb -ffunction-sections -fdata-sections -g -fno-common -fmessage-length=0 -specs=nosys.specs -specs=nano.specs")

include_directories(include drivers/STM32F4xx_HAL_Driver/Inc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-gc-sections -T ${LINKER_SCRIPT}")

set(PLATFORM_DIRECTORY platform/stm32f429-core)
file(GLOB_RECURSE PLATFORM_SOURCES "${PLATFORM_DIRECTORY}/src/*.*" "${PLATFORM_DIRECTORY}/include/*.*")

file(GLOB_RECURSE MIDDLEWARES_SOURCES
        "${PLATFORM_DIRECTORY}/middlewares/FreeRTOS/Source/portable/GCC/ARM_CM4F/*.*"
        "${PLATFORM_DIRECTORY}/middlewares/FreeRTOS/Source/CMSIS_RTOS/*.*"
        "${PLATFORM_DIRECTORY}/middlewares/FreeRTOS/Source/CMSIS_RTOS/*.c"
        "${PLATFORM_DIRECTORY}/middlewares/FreeRTOS/Source/*.c"
        "${PLATFORM_DIRECTORY}/middlewares/LwIP/src/*.*"
        "${PLATFORM_DIRECTORY}/middlewares/LwIP/system/OS/*.*"
        "${PLATFORM_DIRECTORY}/middlewares/mbedTLS/library/*.*")

set(HAL_DEPENDENCIES cortex dma dma_ex eth flash flash_ex flash_ramfunc gpio hcd
        i2c i2c_ex i2s i2s_ex pcd pcd_ex pwr pwr_ex rcc rcc_ex spi tim tim_ex uart)

string(REGEX REPLACE "([^;]+)" "stm32f4xx_hal_\\1.c" HAL_DEPENDENCIES "${HAL_DEPENDENCIES}")
set(HAL_DEPENDENCIES ${HAL_DEPENDENCIES} stm32f4xx_hal.c stm32f4xx_ll_usb)
string(REGEX REPLACE "([^;]+)" "${PLATFORM_DIRECTORY}/drivers/STM32F4xx_HAL_Driver/Src/\\1" HAL_DEPENDENCIES "${HAL_DEPENDENCIES}")
set(PLATFORM_SOURCES ${PLATFORM_SOURCES} ${MIDDLEWARES_SOURCES} ${HAL_DEPENDENCIES})
message("${PLATFORM_SOURCES}")

set(SOURCES ${SOURCES} ${PLATFORM_SOURCES})

include_directories(${PLATFORM_DIRECTORY}/drivers/STM32F4xx_HAL_Driver/Inc)
include_directories(${PLATFORM_DIRECTORY}/drivers/CMSIS/Include)
include_directories(${PLATFORM_DIRECTORY}/drivers/CMSIS/Device/ST/STM32F4xx/Include)
include_directories(${PLATFORM_DIRECTORY}/middlewares/FreeRTOS/Source/include)
include_directories(${PLATFORM_DIRECTORY}/middlewares/FreeRTOS/Source/CMSIS_RTOS)
include_directories(${PLATFORM_DIRECTORY}/middlewares/FreeRTOS/Source/portable/GCC/ARM_CM4F)
include_directories(${PLATFORM_DIRECTORY}/middlewares/LwIP/src/include)
include_directories(${PLATFORM_DIRECTORY}/middlewares/LwIP/system)
include_directories(${PLATFORM_DIRECTORY}/middlewares/mbedTLS/include)
include_directories(${PLATFORM_DIRECTORY}/include)

add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})
target_link_libraries(${PROJECT_NAME}.elf c m)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT} -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map")

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")

add_custom_target(UPLOAD
        arm-none-eabi-gdb -iex "target remote tcp:127.0.0.1:3333"
        -iex "monitor program $<TARGET_FILE:${PROJECT_NAME}.elf>"
        -iex "monitor reset init"
        -iex "disconnect" -iex "quit")

add_custom_target(firmware DEPENDS ${PROJECT_NAME}.elf)