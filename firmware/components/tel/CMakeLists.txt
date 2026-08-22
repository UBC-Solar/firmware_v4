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

set(SUNLITE_OTA_TARGET_ID "0x54454C45" CACHE STRING "32-bit Sunlite OTA target ID")
set(SUNLITE_OTA_HARDWARE_REVISION "1" CACHE STRING "TEL hardware revision")
set(SUNLITE_OTA_FIRMWARE_VERSION "1" CACHE STRING "Monotonic OTA firmware version")
set(SUNLITE_OTA_DISPLAY_VERSION "dev-${SUNLITE_OTA_FIRMWARE_VERSION}" CACHE STRING "Human-readable firmware version")
set(SUNLITE_OTA_MINIMUM_BOOTLOADER_VERSION "1" CACHE STRING "Minimum bootloader version")
set(SUNLITE_OTA_PUBLIC_KEY_FILE "" CACHE FILEPATH "Ed25519 public PEM compiled into the bootloader")

string(TOLOWER "${CMAKE_BUILD_TYPE}" SUNLITE_OTA_BUILD_TYPE_LOWER)
option(TEL_OTA_ENABLE_TEST_BYPASS
    "Bypass TEL's OTA vehicle-state interlock (Debug bench use only)"
    OFF)
if(TEL_OTA_ENABLE_TEST_BYPASS AND
   SUNLITE_OTA_BUILD_TYPE_LOWER STREQUAL "debug")
    set(TEL_OTA_ENABLE_TEST_BYPASS_VALUE 1)
    message(WARNING
        "TEL OTA vehicle-state interlock is bypassed; do not deploy this build on a vehicle")
else()
    set(TEL_OTA_ENABLE_TEST_BYPASS_VALUE 0)
    if(TEL_OTA_ENABLE_TEST_BYPASS)
        message(STATUS
            "Ignoring TEL_OTA_ENABLE_TEST_BYPASS outside a Debug build")
    endif()
endif()

set(SUNLITE_OTA_BOOTLOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../common/bootloader)
set(SUNLITE_OTA_MONOCYPHER_DIR ${SUNLITE_OTA_BOOTLOADER_DIR}/third_party/monocypher)
set(SUNLITE_OTA_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(SUNLITE_OTA_PUBLIC_KEY_HEADER ${SUNLITE_OTA_GENERATED_DIR}/sunlite_ota_public_key.h)
file(MAKE_DIRECTORY ${SUNLITE_OTA_GENERATED_DIR})

if(SUNLITE_OTA_PUBLIC_KEY_FILE)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${SUNLITE_OTA_PUBLIC_KEY_FILE}")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/export_ed25519_public_key.py
            --input ${SUNLITE_OTA_PUBLIC_KEY_FILE}
            --output ${SUNLITE_OTA_PUBLIC_KEY_HEADER}
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(SUNLITE_OTA_PUBLIC_KEY_CONFIGURED 1)
else()
    message(WARNING "SUNLITE_OTA_PUBLIC_KEY_FILE is unset; TEL OTA will fail closed until a key is configured")
    file(WRITE ${SUNLITE_OTA_PUBLIC_KEY_HEADER}
        "#ifndef SUNLITE_OTA_PUBLIC_KEY_H\n#define SUNLITE_OTA_PUBLIC_KEY_H\n"
        "#include <stdint.h>\nstatic const uint8_t sunlite_ota_public_key[32] = {0};\n"
        "#endif\n")
    set(SUNLITE_OTA_PUBLIC_KEY_CONFIGURED 0)
endif()

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cube/cmake/stm32cubemx)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -T "${CMAKE_CURRENT_SOURCE_DIR}/cube/STM32F103XX_FLASH.ld"
)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/app/can_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/tel_ota_safety.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/rtc_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/diagnostics.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app/telemetry_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/imu_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/can_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/rtc_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/driver/telemetry_driver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/CAN_comms/CAN_comms.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/sunlite_ota_app.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/sunlite_ota_can.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/sunlite_ota_can_transport.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/sunlite_ota_protocol.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/bootloader_boot_request.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/debug_io
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/CAN_comms
    ${SUNLITE_OTA_BOOTLOADER_DIR}
    ${SUNLITE_OTA_GENERATED_DIR}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    SUNLITE_OTA_TARGET_ID=${SUNLITE_OTA_TARGET_ID}U
    SUNLITE_OTA_HARDWARE_REVISION=${SUNLITE_OTA_HARDWARE_REVISION}U
    SUNLITE_OTA_FIRMWARE_VERSION=${SUNLITE_OTA_FIRMWARE_VERSION}U
    SUNLITE_OTA_PUBLIC_KEY_CONFIGURED=${SUNLITE_OTA_PUBLIC_KEY_CONFIGURED}
    TEL_OTA_ENABLE_TEST_BYPASS=${TEL_OTA_ENABLE_TEST_BYPASS_VALUE}
    SUNLITE_OTA_SLOT_SIZE_BYTES=225280U
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../common/bootloader/bootloader_target.cmake)
add_stm32f103_bootloader_target(${CMAKE_PROJECT_NAME}_bootloader ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${CMAKE_PROJECT_NAME}_bootloader PRIVATE
    ${SUNLITE_OTA_BOOTLOADER_DIR}/bootloader_tel.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/bootloader_boot_request.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/bootloader_crc32.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/bootloader_flash.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/bootloader_metadata.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/bootloader_sha256.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/sunlite_ota_bootloader_engine.c
    ${SUNLITE_OTA_BOOTLOADER_DIR}/sunlite_ota_protocol.c
    ${SUNLITE_OTA_MONOCYPHER_DIR}/monocypher.c
    ${SUNLITE_OTA_MONOCYPHER_DIR}/monocypher-ed25519.c
)
target_include_directories(${CMAKE_PROJECT_NAME}_bootloader PRIVATE
    ${SUNLITE_OTA_GENERATED_DIR}
    ${SUNLITE_OTA_MONOCYPHER_DIR}
)
target_compile_definitions(${CMAKE_PROJECT_NAME}_bootloader PRIVATE
    SUNLITE_OTA_TARGET_ID=${SUNLITE_OTA_TARGET_ID}U
    SUNLITE_OTA_HARDWARE_REVISION=${SUNLITE_OTA_HARDWARE_REVISION}U
    SUNLITE_OTA_PUBLIC_KEY_CONFIGURED=${SUNLITE_OTA_PUBLIC_KEY_CONFIGURED}
    BOOTLOADER_APP_MAX_SIZE_BYTES=225280U
    BOOTLOADER_METADATA_PAGE_ADDRESS=0x0803F000U
    BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS=0x0803F800U
)
target_compile_options(${CMAKE_PROJECT_NAME}_bootloader PRIVATE -Os)

file(GENERATE
    OUTPUT "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>.ota.json"
    CONTENT "{\n  \"schema\": 1,\n  \"targetId\": \"${SUNLITE_OTA_TARGET_ID}\",\n  \"hardwareRevisionMin\": ${SUNLITE_OTA_HARDWARE_REVISION},\n  \"hardwareRevisionMax\": ${SUNLITE_OTA_HARDWARE_REVISION},\n  \"firmwareVersion\": ${SUNLITE_OTA_FIRMWARE_VERSION},\n  \"displayVersion\": \"${SUNLITE_OTA_DISPLAY_VERSION}\",\n  \"minimumBootloaderVersion\": ${SUNLITE_OTA_MINIMUM_BOOTLOADER_VERSION}\n}\n"
)
