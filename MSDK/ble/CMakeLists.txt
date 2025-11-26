
cmake_minimum_required(VERSION 3.15)

add_library(ble_app_profile_api INTERFACE)

add_library(ble_app_profile)

target_compile_options(ble_app_profile
    PRIVATE
        -w
)

file(GLOB_RECURSE SRC_BLE_APP  ${CMAKE_CURRENT_SOURCE_DIR}/app/*.c)
file(GLOB_RECURSE SRC_BLE_PROFILE  ${CMAKE_CURRENT_SOURCE_DIR}/profile/*.c)

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

HEADER_DIRECTORIES(header_list)

list(REMOVE_ITEM SRC_BLE_APP "${CMAKE_CURRENT_SOURCE_DIR}/app/atcmd_ble.c")
list(REMOVE_ITEM SRC_BLE_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/profile/hogp/ble_hogpd.c")

target_sources(ble_app_profile
    PRIVATE
        ${SRC_BLE_APP}
        ${SRC_BLE_PROFILE}
)

target_include_directories(ble_app_profile
    PRIVATE
        ${header_list}
)

target_include_directories(ble_app_profile_api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/app/>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/profile/>
        $<BUILD_INTERFACE:${header_list}>
)

target_link_libraries(ble_app_profile
    ble_app_profile_api
    bsp_api
    riscv_api
    util_api
    rtos_api
    wifi_mgmt_api
    lwIP_api
    wifi_api
    mbedtls_api
    gd32vw55x_ble_api
)

add_lib_api(ble_app_profile_api)
add_lib(ble_app_profile)