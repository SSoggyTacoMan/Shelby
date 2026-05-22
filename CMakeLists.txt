cmake_minimum_required(VERSION 3.13)

# Default to Pico W (Sprig uses Pico W / RP2040 + CYW43)
set(PICO_BOARD "pico_w" CACHE STRING "Pico board")

if(NOT DEFINED ENV{PICO_SDK_PATH})
  message(FATAL_ERROR "PICO_SDK_PATH is not set. Export PICO_SDK_PATH to your pico-sdk checkout.")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(shelby_native C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

pico_sdk_init()

add_executable(shelby_native
  main.cpp
  Display.cpp
)

target_link_libraries(shelby_native
  pico_stdlib
  hardware_spi
  pico_cyw43_arch_lwip_threadsafe_background
  pico_lwip
  pico_mbedtls
  pico_lwip_mbedtls
)

target_include_directories(shelby_native PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(shelby_native 1)
pico_enable_stdio_uart(shelby_native 0)

pico_add_extra_outputs(shelby_native)
