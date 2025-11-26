
cmake_minimum_required(VERSION 3.15)

add_library(bsp_api INTERFACE)

add_library(bsp)

target_sources(bsp
    PRIVATE
        bsp.c
        gd32vw55x_platform.c
        init_rom.c
        plf_assert.c
        wakelock.c
        dma/dma.c
        nvds/nvds_flash.c
        raw_flash/raw_flash_api.c
        time/systime.c
        trng/trng.c
        uart/log_uart.c
        uart/uart.c
        uart/trace_uart.c
        uart/ble_uart.c
        dsp.c
        wdt/wdt.c
)

target_include_directories(bsp_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dma>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nvds>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/raw_flash>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/reg>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rf>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/time>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/trng>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/uart>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wdt>
)

target_link_libraries(bsp
    bsp_api
    riscv_api
    util_api
    rtos_api
    wifi_mgmt_api
    lwIP_api
    wifi_api
    gd32vw55x_ble_api
    mbedtls_api
)

add_lib_api(bsp_api)
add_lib(bsp)