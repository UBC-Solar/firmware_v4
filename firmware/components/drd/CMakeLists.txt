cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME drd)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wall
    -g3
    # -Werror
)


# Add STM32CubeMX generated sources
add_subdirectory(cube/cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
file(GLOB_RECURSE COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../common/*/*.c")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/app/evan_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/drive_state.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/soc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/adc_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/evan_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/gpio_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/can_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/interrupts.c
    ${COMMON_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/debug_io
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/CAN_comms
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    DEBUG
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
