cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico_w) 
# Always include it
include(pico_sdk_import.cmake)

# Project's name (Replace my_DE3_Project with your own project's name)
project(my_DE3_Project)

# SDK Initialization - Mandatory
pico_sdk_init()

# C/C++ project files
add_executable(my_DE3_Project
    main.c
)

# pico_stdlib library. You can add more if they are needed
target_link_libraries(my_DE3_Project 
pico_stdlib
pico_rand 
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(my_DE3_Project 1)
pico_enable_stdio_uart(my_DE3_Project 0)

# Need to generate UF2 file for upload to RP2040
pico_add_extra_outputs(my_DE3_Project)