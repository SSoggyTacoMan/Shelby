cmake_minimum_required(VERSION 3.13)

if(DEFINED ENV{PICO_SDK_PATH})
    include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
else()
    message(FATAL_ERROR "PICO_SDK_PATH is not set. Export it before configuring.")
endif()

project(shelby_native C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

pico_sdk_init()

add_executable(shelby_native
    /home/runner/work/Shelby/Shelby/src_cpp/main.cpp
)

target_link_libraries(shelby_native
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_cyw43_arch_lwip_threadsafe_background
)

target_include_directories(shelby_native PRIVATE
    /home/runner/work/Shelby/Shelby/src_cpp
)

pico_enable_stdio_usb(shelby_native 0)
pico_enable_stdio_uart(shelby_native 1)

pico_add_extra_outputs(shelby_native)
