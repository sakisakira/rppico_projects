# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 1.5.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(button_volume_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(button_and_volume)

target_sources(button_and_volume PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/button_and_volume.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(button_and_volume PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

#pico_enable_stdio_usb(button_and_volume 0)
#pico_enable_stdio_uart(button_and_volume 0)

target_link_libraries(button_and_volume PUBLIC pico_stdlib pico_unique_id tinyusb_device tinyusb_board hardware_adc)

pico_add_extra_outputs(button_and_volume)
