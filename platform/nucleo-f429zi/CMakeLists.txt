set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/platform/nucleo-f429zi/flash.ld)

include_directories(platform/nucleo-f429zi/include)

include(platform/stm32f429-core/CMakeLists.txt)