include_guard(GLOBAL)

# Bootloader executable configuration for STM32F103 boards.
# Uses a separate linker script and emits a raw binary for SWD provisioning.

# target_name: Bootloader executable target to create.
# board_dir: Board project containing Cube startup, syscalls, and HAL targets.
function(add_stm32f103_bootloader_target target_name board_dir)
    set(BOOTLOADER_DIR ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
    set(BOOTLOADER_LINKER_SCRIPT "${BOOTLOADER_DIR}/STM32F103RC_BOOTLOADER_FLASH.ld")

    add_executable(${target_name})

    target_sources(${target_name} PRIVATE
        ${board_dir}/cube/startup_stm32f103xe.s
        ${board_dir}/cube/Core/Src/syscalls.c
        ${board_dir}/cube/Core/Src/sysmem.c
        ${BOOTLOADER_DIR}/bootloader.c
        ${BOOTLOADER_DIR}/bootloader_boot_request.c
    )

    target_include_directories(${target_name} PRIVATE
        ${BOOTLOADER_DIR}
    )

    target_link_libraries(${target_name}
        stm32cubemx
        STM32_Drivers
        ${TOOLCHAIN_LINK_LIBRARIES}
    )

    target_link_options(${target_name} PRIVATE
        -T "${BOOTLOADER_LINKER_SCRIPT}"
    )

    set_target_properties(${target_name} PROPERTIES
        ADDITIONAL_CLEAN_FILES ${target_name}.map
    )

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${target_name}.bin
        COMMENT "Generating ${target_name}.bin"
    )
endfunction()

# Configure the shared CAN update sources and image metadata.
# app_target / bootloader_target: Existing executable targets to configure.
# board_name: Display name used in build messages and version defaults.
# target_id: Firmware protocol target identifier for this board.
# can_node_id / can_remap: CAN address and STM32 CAN pin-remap selection.
function(configure_fw_update_remote_target
         app_target
         bootloader_target
         board_name
         target_id
         can_node_id
         can_remap)
    set(FW_UPDATE_FIRMWARE_VERSION "1" CACHE STRING "${board_name} application firmware version")

    string(TOLOWER "${CMAKE_BUILD_TYPE}" ota_build_type)
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

    set(ota_bootloader_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    target_sources(${app_target} PRIVATE
        "${ota_bootloader_dir}/fw_update_can.c"
        "${ota_bootloader_dir}/fw_update_protocol.c"
        "${ota_bootloader_dir}/bootloader_boot_request.c"
    )
    target_include_directories(${app_target} PRIVATE
        "${ota_bootloader_dir}"
    )
    target_compile_definitions(${app_target} PRIVATE
        FW_UPDATE_TARGET_ID=${target_id}U
        FW_UPDATE_FIRMWARE_VERSION=${FW_UPDATE_FIRMWARE_VERSION}U
        FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE=${ota_allow_unsafe_bench}
        FW_UPDATE_CAN_NODE_ID=${can_node_id}U
        FW_UPDATE_CAN_REMAP=${can_remap}
    )

    target_sources(${bootloader_target} PRIVATE
        "${ota_bootloader_dir}/bootloader_storage.c"
        "${ota_bootloader_dir}/fw_update_bootloader_engine.c"
        "${ota_bootloader_dir}/fw_update_can.c"
        "${ota_bootloader_dir}/fw_update_protocol.c"
    )
    target_compile_definitions(${bootloader_target} PRIVATE
        BOOTLOADER_BUILD
        FW_UPDATE_TARGET_ID=${target_id}U
        FW_UPDATE_CAN_NODE_ID=${can_node_id}U
        FW_UPDATE_CAN_REMAP=${can_remap}
    )
    # Optimize the bootloader across modules in Release builds.
    target_compile_options(${bootloader_target} PRIVATE -Os "$<$<CONFIG:Release>:-flto>")
    target_link_options(${bootloader_target} PRIVATE "$<$<CONFIG:Release>:-flto>")

    file(GENERATE
        OUTPUT "$<TARGET_FILE:${app_target}>.ota.json"
        CONTENT "{\n  \"schema\": 2,\n  \"targetId\": \"${target_id}\",\n  \"firmwareVersion\": ${FW_UPDATE_FIRMWARE_VERSION}\n}\n"
    )
endfunction()
