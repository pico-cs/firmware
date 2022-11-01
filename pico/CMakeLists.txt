cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

#include_directories("../nanopb" , "msg")

project(cs C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(cs
    cs.c
    io.c
    cmd.c
    exe.c
    dcc.c
    rbuf.c
    channel.c
)

pico_generate_pio_header(cs ${CMAKE_CURRENT_LIST_DIR}/dcc_tx.pio)

pico_enable_stdio_usb(cs 1)
pico_enable_stdio_uart(cs 1)

pico_add_extra_outputs(cs)
target_link_libraries(cs
    pico_stdlib
    pico_multicore
    pico_sync
    hardware_pio
    hardware_adc
    pico_bootsel_via_double_reset
)
