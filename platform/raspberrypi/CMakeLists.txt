SET(CMAKE_FIND_ROOT_PATH $ENV{RASPBERRY_SYSROOT})

# Use our definitions for compiler tools
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Search for libraries and headers in the target directories only
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(PLATFORM_DIRECTORY platform/raspberrypi)
file(GLOB_RECURSE RPI_SOURCES "${PLATFORM_DIRECTORY}/src/*.*" "${PLATFORM_DIRECTORY}/include/*.*")

file(GLOB_RECURSE MIDDLEWARES_SOURCES)

set(PLATFORM_SOURCES ${RPI_SOURCES} ${MIDDLEWARES_SOURCES})
message("${PLATFORM_SOURCES}")

set(SOURCES ${SOURCES} ${PLATFORM_SOURCES})

include_directories(${PLATFORM_DIRECTORY}/include)

include(platform/unix/CMakeLists.txt)
remove_definitions(-DUNIX)

# Cross-compiler settings
set(CMAKE_C_COMPILER "$ENV{RASPBERRY_COMPILERS}/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "$ENV{RASPBERRY_COMPILERS}/arm-linux-gnueabihf-g++")
set(CMAKE_ASM_COMPILER "$ENV{RASPBERRY_COMPILERS}/arm-linux-gnueabihf-gcc")
set(CMAKE_AR "$ENV{RASPBERRY_COMPILERS}/arm-linux-gnueabihf-ar")
set(CMAKE_OBJCOPY "$ENV{RASPBERRY_COMPILERS}/arm-linux-gnueabihf-objcopy")
set(CMAKE_OBJDUMP "$ENV{RASPBERRY_COMPILERS}/arm-linux-gnueabihf-objdump")
set(CMAKE_SIZE "$ENV{RASPBERRY_COMPILERS}/arm-linux-gnueabihf-size")