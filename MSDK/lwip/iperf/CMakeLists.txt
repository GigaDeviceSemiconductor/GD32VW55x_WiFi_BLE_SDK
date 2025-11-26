cmake_minimum_required(VERSION 3.15)

add_library(iperf_api INTERFACE)

target_include_directories(iperf_api
INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
add_lib_api(iperf_api)
