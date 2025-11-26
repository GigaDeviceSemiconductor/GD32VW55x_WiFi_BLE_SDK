
set(CMAKE_SYSTEM_NAME Generic)
set(CROSS_COMPILE riscv-nuclei-elf)

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}-gcc)
find_program(CMAKE_CXX_COMPILER ${CROSS_COMPILE}-g++)
find_program(CMAKE_AR ${CROSS_COMPILE}-ar)
find_program(CMAKE_RANLIB ${CROSS_COMPILE}-ranlib)
find_program(CMAKE_LINKER ${CROSS_COMPILE}-linker)
find_program(CMAKE_OBJCOPY ${CROSS_COMPILE}-objcopy)
find_program(CMAKE_OBJDUMP ${CROSS_COMPILE}-objdump)
find_program(CMAKE_STRIP ${CROSS_COMPILE}-strip)

set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/set_extensions.cmake)

if (CONFIG_OS STREQUAL "FREERTOS")
    set(PLATFORM_OS_FREERTOS ON)
    add_definitions(
        -DPLATFORM_OS_FREERTOS
    )
elseif (CONFIG_OS STREQUAL "RTTHREAD")
    set(PLATFORM_OS_RTTHREAD ON)
    add_definitions(
        -DPLATFORM_OS_RTTHREAD
    )
endif()

# Set architecture and CPU
set(SYSTEM_PROCESSOR riscv)
set(SYSTEM_ARCHITECTURE rv32imafcbp)
set(SYSTEM_FP ON)


set(CMAKE_SYSTEM_PROCESSOR ${SYSTEM_PROCESSOR})
set(CMAKE_SYSTEM_ARCHITECTURE ${SYSTEM_ARCHITECTURE})

set(C_FLAGS
    -mcmodel=medlow
    -msmall-data-limit=8
    -msave-restore
    -mabi=ilp32f
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wuninitialized
    -std=c99)

set(CXX_FLAGS
    -mcmodel=medlow
    -msmall-data-limit=8
    -msave-restore
    -mabi=ilp32f
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wuninitialized
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
    -fno-use-cxa-atexit
    -fpermissive
    -fno-jump-tables
    -fno-tree-switch-conversion
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -Wno-register)

set(ASM_FLAGS
    -mcmodel=medlow
    -msmall-data-limit=8
    -msave-restore
    -mabi=ilp32f
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wuninitialized
    -x assembler-with-cpp)

macro(toolchain_reset_compiler_flags)
    set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")

    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C>:${C_FLAGS}>"
        "$<$<COMPILE_LANGUAGE:ASM>:${ASM_FLAGS}>"
        "$<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>"
    )

endmacro()

macro(toolchain_reset_linker_flags)
    set_property(DIRECTORY PROPERTY LINK_OPTIONS "")

    add_link_options(
        -mcmodel=medlow
        -msmall-data-limit=8
        -msave-restore
        -mabi=ilp32f
        -Os
        -fmessage-length=0
        -fsigned-char
        -ffunction-sections
        -fdata-sections
        -fno-common
        -nostartfiles
        -fno-exceptions
        -fno-rtti
        -Xlinker
        --gc-sections
        --specs=nano.specs
        --specs=nosys.specs
        -Wuninitialized
        LINKER:--print-memory-usage
        LINKER:-check-sections
    )

endmacro()

macro(toolchain_set_processor_arch)
    set(CMAKE_SYSTEM_PROCESSOR ${SYSTEM_PROCESSOR})
    set(CMAKE_SYSTEM_ARCHITECTURE ${SYSTEM_ARCHITECTURE})
endmacro()

macro(toolchain_reload_compiler)
    toolchain_set_processor_arch()
    toolchain_reset_compiler_flags()
    toolchain_reset_linker_flags()

    unset(CMAKE_C_FLAGS_INIT)
    unset(CMAKE_ASM_FLAGS_INIT)
    unset(CMAKE_CXX_FLAGS_INIT)

    set(CMAKE_C_FLAGS_INIT "-march=${SYSTEM_ARCHITECTURE}")
    set(CMAKE_CXX_FLAGS_INIT "-march=${SYSTEM_ARCHITECTURE}")
    set(CMAKE_ASM_FLAGS_INIT "-march=${SYSTEM_ARCHITECTURE}")
    #set(CMAKE_C_LINK_FLAGS "-march=${SYSTEM_ARCHITECTURE}")
    set(CMAKE_ASM_LINK_FLAGS "-march=${SYSTEM_ARCHITECTURE}")

    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS_INIT})
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_INIT})
    set(CMAKE_ASM_FLAGS ${CMAKE_ASM_FLAGS_INIT})
endmacro()

toolchain_reload_compiler()

macro(target_add_scatter_file target)
    target_link_options(${target}
        PRIVATE
        -T $<TARGET_OBJECTS:${target}_scatter>
    )

    add_dependencies(${target}
        ${target}_scatter
    )

    add_library(${target}_scatter OBJECT)
    foreach(scatter_file ${ARGN})
        target_sources(${target}_scatter
            PRIVATE
                ${scatter_file}
        )
        # Cmake cannot use generator expressions in the
        # set_source_file_properties command, so instead we just parse the regex
        # for the filename and set the property on all files, regardless of if
        # the generator expression would evaluate to true or not.
        string(REGEX REPLACE ".*>:(.*)>$" "\\1" SCATTER_FILE_PATH "${scatter_file}")
        set_source_files_properties(${SCATTER_FILE_PATH}
            PROPERTIES
            LANGUAGE C
        )
    endforeach()



    target_compile_options(${target}_scatter
        PRIVATE
            -E
            -P
            -xc
    )
endmacro()
