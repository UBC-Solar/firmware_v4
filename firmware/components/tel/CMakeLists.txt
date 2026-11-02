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
set(CMAKE_PROJECT_NAME tel)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cube/cmake/stm32cubemx)

# Extract the pinned local Digi sources into the build directory (offline-safe).
include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(xbee_ansic
    URL ${CMAKE_CURRENT_SOURCE_DIR}/../../common/xbee/xbee_ansic.tar.gz
    URL_HASH SHA256=d84697dc9bc2b8567f15f234360d87249aed8e8f7eb791d2d343eb63935ad910
)
FetchContent_MakeAvailable(xbee_ansic)
add_library(xbee STATIC
    ${xbee_ansic_SOURCE_DIR}/src/xbee/xbee_device.c
    ${xbee_ansic_SOURCE_DIR}/src/xbee/xbee_ipv4.c
    ${xbee_ansic_SOURCE_DIR}/src/util/swapbytes.c
)
target_include_directories(xbee PUBLIC
    ${xbee_ansic_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
)
target_compile_definitions(xbee PUBLIC XBEE_PLATFORM_HEADER="xbee_platform.h")
target_link_libraries(xbee PUBLIC stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/interrupts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/imu_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/canload.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/can_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/rtc_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/diagnostics.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/telemetry_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/gps_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/imu_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/can_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/gpio_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/rtc_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/telemetry_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/gps_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/mcu_health_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/CAN_comms/CAN_comms.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/iwdg/iwdg_common.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/debug_io
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/CAN_comms
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/car_configs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/iwdg
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    xbee

    # Add user defined libraries
)
