cmake_minimum_required(VERSION 3.12)

if (PICO_SDK_PATH_OVERRIDE)
    set(PICO_SDK_PATH ${PICO_SDK_PATH_OVERRIDE})
else()
    set(PICO_SDK_PATH pico-sdk)
endif()

# Pull in SDK (must be before project)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(QuadratureDecoder C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )



add_executable(QuadratureDecoder)

pico_generate_pio_header(QuadratureDecoder ${CMAKE_CURRENT_LIST_DIR}/QuadratureDecoder.pio)

target_sources(QuadratureDecoder PRIVATE
    main.cpp
    QuadratureDecoder.cpp
    )

target_link_libraries(QuadratureDecoder PRIVATE
    pico_stdlib
    hardware_pio hardware_dma
    )
pico_add_extra_outputs(QuadratureDecoder)

pico_enable_stdio_usb(QuadratureDecoder 1)
pico_enable_stdio_uart(QuadratureDecoder 0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
