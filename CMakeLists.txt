cmake_minimum_required(VERSION 3.7)

set(CMAKE_C_COMPILER_WORKS 1) # due to the /usr/bin/cc test compile error

set(OUTPUT_DIRECTORY out/)

project(js-embedded-platform C ASM)

set(PLATFORM "none" CACHE STRING "Build target platform")

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_BOOTSTRAP -DLWM2M_SUPPORT_JSON)

include_directories(include)

file(GLOB_RECURSE SOURCES "drivers/*.*" "src/*.c" "include/*.*")
if (PLATFORM MATCHES "nucleo-f429zi|unix")
    include(platform/${PLATFORM}/CMakeLists.txt)
else()
    message(FATAL_ERROR "Unknown PLATFORM option: ${PLATFORM}. Use cmake . -DPLATFORM=<platform>")
endif()
