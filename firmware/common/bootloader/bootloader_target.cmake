function(add_stm32f103_bootloader_target target_name board_dir)
    set(BOOTLOADER_DIR ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
    set(BOOTLOADER_LINKER_SCRIPT "${BOOTLOADER_DIR}/STM32F103RC_BOOTLOADER_FLASH.ld")

    if(ARGC GREATER 2)
        set(BOOTLOADER_LINKER_SCRIPT "${ARGV2}")
    endif()

    add_executable(${target_name})

    target_sources(${target_name} PRIVATE
        ${board_dir}/cube/startup_stm32f103xe.s
        ${board_dir}/cube/Core/Src/syscalls.c
        ${board_dir}/cube/Core/Src/sysmem.c
        ${BOOTLOADER_DIR}/bootloader.c
        ${BOOTLOADER_DIR}/bootloader_interrupts.c
        ${BOOTLOADER_DIR}/bootloader_main.c
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

function(configure_stm32f103rb_bootloader_target target_name)
    set(BOOTLOADER_DIR ${CMAKE_CURRENT_FUNCTION_LIST_DIR})

    target_sources(${target_name} PRIVATE
        ${BOOTLOADER_DIR}/bootloader_nucleo_f103rb.c
        ${BOOTLOADER_DIR}/bootloader_crc32.c
        ${BOOTLOADER_DIR}/bootloader_flash.c
    )

    target_compile_definitions(${target_name} PRIVATE
        BOOTLOADER_FLASH_SIZE_BYTES=131072U
        BOOTLOADER_FLASH_PAGE_SIZE_BYTES=1024U
        BOOTLOADER_SRAM_SIZE_BYTES=20480U
    )
endfunction()
