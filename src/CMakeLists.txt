cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_CS_DEFAULT_RBUF_SIZE 128)
    
if (DEFINED ENV{PICO_CS_RBUF_SIZE} AND (NOT PICO_CS_RBUF_SIZE))
    set(PICO_CS_RBUF_SIZE $ENV{PICO_CS_RBUF_SIZE})
    message("Using PICO_CS_RUF_SIZE from environment ('${PICO_CS_RBUF_SIZE}')")
endif()

if (NOT PICO_CS_RBUF_SIZE)
    set(PICO_CS_RBUF_SIZE ${PICO_CS_DEFAULT_RBUF_SIZE})
    message("Using PICO_CS_RBUF_SIZE default ('${PICO_CS_DEFAULT_RBUF_SIZE}')")
endif()

if (PICO_CS_RBUF_SIZE LESS 64)
    message(
        FATAL_ERROR
        "Refresh buffer size minimum is 64. Please use valid PICO_CS_RBUF_SIZE value between 64 and 255."
    )
endif()

if (PICO_CS_RBUF_SIZE GREATER 255)
    message(
        FATAL_ERROR
        "Refresh buffer size maximum is 255. Please use valid PICO_CS_RBUF_SIZE value between 64 and 255."
    )
endif()

if (PICO_BOARD STREQUAL "pico")
    project(cs C CXX ASM)
    pico_sdk_init()

    add_executable(cs
        cs.c
        common.c
        dcc_tx.c
        prot.c
        board.c
        board_pico.c
        loop_pico.c
        io.c
        cmd.c
        mt.c
        dcc.c
        rbuf.c
        channel.c
    )

    target_compile_definitions(cs PRIVATE
        PICO_W=0
        RBUF_SIZE=${PICO_CS_RBUF_SIZE}
        RBUF_SIZE_STRING=\"${PICO_CS_RBUF_SIZE}\"
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
endif()

if (PICO_BOARD STREQUAL "pico_w")

    set(PICO_CS_DEFAULT_TCP_PORT 4242)

    if (DEFINED ENV{PICO_CS_WIFI_SSID} AND (NOT PICO_CS_WIFI_SSID))
        set(PICO_CS_WIFI_SSID $ENV{PICO_CS_WIFI_SSID})
        message("Using PICO_CS_WIFI_SSID from environment ('${PICO_CS_WIFI_SSID}')")
    endif()

    if (DEFINED ENV{PICO_CS_WIFI_PASSWORD} AND (NOT PICO_CS_WIFI_PASSWORD))
        set(PICO_CS_WIFI_PASSWORD $ENV{PICO_CS_WIFI_PASSWORD})
        message("Using PICO_CS_WIFI_PASSWORD from environment ('${PICO_CS_WIFI_PASSWORD}')")
    endif()

    if (DEFINED ENV{PICO_CS_TCP_PORT} AND (NOT PICO_CS_TCP_PORT))
        set(PICO_CS_TCP_PORT $ENV{PICO_CS_TCP_PORT})
        message("Using PICO_CS_TCP_PORT from environment ('${PICO_CS_TCP_PORT}')")
    endif()

    if (NOT PICO_CS_WIFI_SSID)
        message(
            FATAL_ERROR
            "WiFi SSID was not specified. Please set PICO_CS_WIFI_SSID."
        )
    endif()

    if (NOT PICO_CS_WIFI_PASSWORD)
        message(
            FATAL_ERROR
            "WiFi password was not specified. Please set PICO_CS_WIFI_PASSWORD."
        )
    endif()

    if (NOT PICO_CS_TCP_PORT)
        set(PICO_CS_TCP_PORT ${PICO_CS_DEFAULT_TCP_PORT})
        message("Using PICO_CS_TCP_PORT default ('${PICO_CS_TCP_PORT}')")
    endif()
    
    project(cs_w C CXX ASM)
    pico_sdk_init()

    add_executable(cs_w
        cs.c
        common.c
        dcc_tx.c
        prot.c
        board.c
        board_pico_w.c
        loop_pico_w.c
        tcp_server.c
        io.c
        cmd.c
        mt.c
        dcc.c
        rbuf.c
        channel.c
    )

    target_compile_definitions(cs_w PRIVATE
        PICO_W=1
        RBUF_SIZE=${PICO_CS_RBUF_SIZE}
        RBUF_SIZE_STRING=\"${PICO_CS_RBUF_SIZE}\"
        WIFI_SSID=\"${PICO_CS_WIFI_SSID}\"
        WIFI_PASSWORD=\"${PICO_CS_WIFI_PASSWORD}\"
        TCP_PORT=${PICO_CS_TCP_PORT}
        TCP_PORT_STRING=\"${PICO_CS_TCP_PORT}\"
    )
    target_include_directories(cs_w PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/..  # for our common lwipopts
    )

    pico_generate_pio_header(cs_w ${CMAKE_CURRENT_LIST_DIR}/dcc_tx.pio)

    pico_enable_stdio_usb(cs_w 1)
    pico_enable_stdio_uart(cs_w 1)

    pico_add_extra_outputs(cs_w)
    target_link_libraries(cs_w
        pico_stdlib
        pico_cyw43_arch_lwip_poll
        pico_multicore
        pico_sync
        hardware_pio
        hardware_adc
        pico_bootsel_via_double_reset
    )
endif()
