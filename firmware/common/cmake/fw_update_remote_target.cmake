function(configure_fw_update_remote_target
         app_target
         bootloader_target
         board_name
         target_id
         can_node_id
         can_remap)
    set(FW_UPDATE_HARDWARE_REVISION "1" CACHE STRING "${board_name} hardware revision")
    set(FW_UPDATE_FIRMWARE_VERSION "1" CACHE STRING "${board_name} monotonic OTA firmware version")
    string(TOLOWER "${board_name}" board_name_lower)
    set(FW_UPDATE_DISPLAY_VERSION
        "${board_name_lower}-dev-${FW_UPDATE_FIRMWARE_VERSION}"
        CACHE STRING "${board_name} human-readable firmware version")
    set(FW_UPDATE_MINIMUM_BOOTLOADER_VERSION "1" CACHE STRING "Minimum bootloader version")
    set(FW_UPDATE_PUBLIC_KEY_FILE "" CACHE FILEPATH "Ed25519 public PEM compiled into ${board_name}")

    string(TOLOWER "${CMAKE_BUILD_TYPE}" ota_build_type)
    option(FW_UPDATE_ALLOW_UNSIGNED_BENCH "Accept unsigned firmware on a controlled Debug bench" OFF)
    set(ota_unsigned_bench 0)
    if(FW_UPDATE_ALLOW_UNSIGNED_BENCH)
        if(NOT ota_build_type STREQUAL "debug")
            message(FATAL_ERROR "FW_UPDATE_ALLOW_UNSIGNED_BENCH requires Debug")
        endif()
        set(ota_unsigned_bench 1)
        message(WARNING "${board_name}: unsigned bench firmware updates enabled")
    endif()
    set(ota_bench_default OFF)
    if(ota_build_type STREQUAL "debug")
        set(ota_bench_default ON)
    endif()
    option(FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE
        "Allow OTA without a board safety interlock (bench use only)"
        ${ota_bench_default})
    if(FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE AND
       ota_build_type STREQUAL "debug")
        set(ota_allow_unsafe_bench 1)
        message(WARNING
            "${board_name} permits OTA without a safety interlock; do not deploy this build on a vehicle")
    else()
        set(ota_allow_unsafe_bench 0)
        if(FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE)
            message(STATUS
                "Ignoring FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE outside a Debug build")
        endif()
    endif()

    set(ota_bootloader_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../bootloader")
    set(ota_monocypher_dir "${ota_bootloader_dir}/third_party/monocypher")
    set(ota_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
    set(ota_public_key_header "${ota_generated_dir}/fw_update_public_key.h")
    file(MAKE_DIRECTORY "${ota_generated_dir}")

    if(FW_UPDATE_PUBLIC_KEY_FILE)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${FW_UPDATE_PUBLIC_KEY_FILE}")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        execute_process(
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../../tools/export_ed25519_public_key.py"
                --input "${FW_UPDATE_PUBLIC_KEY_FILE}"
                --output "${ota_public_key_header}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        set(ota_public_key_configured 1)
    else()
        if(NOT ota_unsigned_bench)
            message(WARNING "FW_UPDATE_PUBLIC_KEY_FILE is unset; ${board_name} OTA will fail closed until a key is configured")
        endif()
        file(WRITE "${ota_public_key_header}"
            "#ifndef FW_UPDATE_PUBLIC_KEY_H\n#define FW_UPDATE_PUBLIC_KEY_H\n"
            "#include <stdint.h>\nstatic const uint8_t fw_update_public_key[32] = {0};\n"
            "#endif\n")
        set(ota_public_key_configured 0)
    endif()

    target_sources(${app_target} PRIVATE
        "${ota_bootloader_dir}/fw_update_can_app.c"
        "${ota_bootloader_dir}/fw_update_can.c"
        "${ota_bootloader_dir}/fw_update_can_transport.c"
        "${ota_bootloader_dir}/fw_update_protocol.c"
        "${ota_bootloader_dir}/bootloader_boot_request.c"
    )
    target_include_directories(${app_target} PRIVATE
        "${ota_bootloader_dir}"
        "${ota_generated_dir}"
    )
    target_compile_definitions(${app_target} PRIVATE
        FW_UPDATE_TARGET_ID=${target_id}U
        FW_UPDATE_HARDWARE_REVISION=${FW_UPDATE_HARDWARE_REVISION}U
        FW_UPDATE_FIRMWARE_VERSION=${FW_UPDATE_FIRMWARE_VERSION}U
        FW_UPDATE_ALLOW_UNSIGNED_BENCH=${ota_unsigned_bench}
        FW_UPDATE_PUBLIC_KEY_CONFIGURED=${ota_public_key_configured}
        FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE=${ota_allow_unsafe_bench}
        FW_UPDATE_SLOT_SIZE_BYTES=225280U
        FW_UPDATE_CAN_NODE_ID=${can_node_id}U
        FW_UPDATE_CAN_REMAP=${can_remap}
    )

    target_sources(${bootloader_target} PRIVATE
        "${ota_bootloader_dir}/bootloader_can.c"
        "${ota_bootloader_dir}/bootloader_boot_request.c"
        "${ota_bootloader_dir}/bootloader_crc32.c"
        "${ota_bootloader_dir}/bootloader_flash.c"
        "${ota_bootloader_dir}/bootloader_metadata.c"
        "${ota_bootloader_dir}/bootloader_sha256.c"
        "${ota_bootloader_dir}/fw_update_bootloader_engine.c"
        "${ota_bootloader_dir}/fw_update_can.c"
        "${ota_bootloader_dir}/fw_update_can_transport.c"
        "${ota_bootloader_dir}/fw_update_protocol.c"
        "${ota_monocypher_dir}/monocypher.c"
        "${ota_monocypher_dir}/monocypher-ed25519.c"
    )
    target_include_directories(${bootloader_target} PRIVATE
        "${ota_generated_dir}"
        "${ota_monocypher_dir}"
    )
    target_compile_definitions(${bootloader_target} PRIVATE
        FW_UPDATE_TARGET_ID=${target_id}U
        FW_UPDATE_HARDWARE_REVISION=${FW_UPDATE_HARDWARE_REVISION}U
        FW_UPDATE_ALLOW_UNSIGNED_BENCH=${ota_unsigned_bench}
        FW_UPDATE_PUBLIC_KEY_CONFIGURED=${ota_public_key_configured}
        FW_UPDATE_CAN_NODE_ID=${can_node_id}U
        FW_UPDATE_CAN_REMAP=${can_remap}
        BOOTLOADER_APP_MAX_SIZE_BYTES=225280U
        BOOTLOADER_METADATA_PAGE_ADDRESS=0x0803F000U
        BOOTLOADER_METADATA_BACKUP_PAGE_ADDRESS=0x0803F800U
    )
    target_compile_options(${bootloader_target} PRIVATE -Os)

    file(GENERATE
        OUTPUT "$<TARGET_FILE:${app_target}>.ota.json"
        CONTENT "{\n  \"schema\": 1,\n  \"targetId\": \"${target_id}\",\n  \"hardwareRevisionMin\": ${FW_UPDATE_HARDWARE_REVISION},\n  \"hardwareRevisionMax\": ${FW_UPDATE_HARDWARE_REVISION},\n  \"firmwareVersion\": ${FW_UPDATE_FIRMWARE_VERSION},\n  \"displayVersion\": \"${FW_UPDATE_DISPLAY_VERSION}\",\n  \"minimumBootloaderVersion\": ${FW_UPDATE_MINIMUM_BOOTLOADER_VERSION}\n}\n"
    )
endfunction()
