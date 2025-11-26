cmake_minimum_required(VERSION 3.15)

add_subdirectory(lwip-2.2.0)

if (NOT CONFIG_STATIC_LIB_BUILDED)
    add_subdirectory(iperf3)
endif()

add_subdirectory(iperf)
