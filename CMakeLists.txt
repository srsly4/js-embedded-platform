cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS} -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,-gc-sections -T ${LINKER_SCRIPT}")

project(js-embedded-platform C ASM)

set(PLATFORM "none" CACHE STRING "Build target platform")

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.*")

if (PLATFORM MATCHES "nucleo-f429zi")
    include(platform/${PLATFORM}/CMakeLists.txt)
else()
    message(FATAL_ERROR "Unknown PLATFORM option: ${PLATFORM}. Use cmake . -DPLATFORM=<platform>")
endif()
