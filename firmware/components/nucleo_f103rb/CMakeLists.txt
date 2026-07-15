cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME nucleo_f103rb)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

set(MDI_CUBE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../mdi/cube)
set(BOOTLOADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../common/bootloader)

set(NUCLEO_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${MDI_CUBE_DIR}/Core/Inc
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    ${MDI_CUBE_DIR}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
    ${MDI_CUBE_DIR}/Drivers/CMSIS/Include
)

set(NUCLEO_DEFINES
    USE_HAL_DRIVER
    STM32F103xE
    $<$<CONFIG:Debug>:DEBUG>
)

add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${NUCLEO_INCLUDE_DIRS})
target_compile_definitions(stm32cubemx INTERFACE ${NUCLEO_DEFINES})

add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE
    ${MDI_CUBE_DIR}/Core/Src/system_stm32f1xx.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
    ${MDI_CUBE_DIR}/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
)
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)

add_executable(${CMAKE_PROJECT_NAME}_app)
target_sources(${CMAKE_PROJECT_NAME}_app PRIVATE
    ${MDI_CUBE_DIR}/startup_stm32f103xe.s
    ${MDI_CUBE_DIR}/Core/Src/syscalls.c
    ${MDI_CUBE_DIR}/Core/Src/sysmem.c
    app/nucleo_app_main.c
)
target_link_libraries(${CMAKE_PROJECT_NAME}_app
    stm32cubemx
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
)
target_link_options(${CMAKE_PROJECT_NAME}_app PRIVATE
    -T "${BOOTLOADER_DIR}/STM32F103RB_APP_FLASH.ld"
)
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}_app POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}_app> ${CMAKE_PROJECT_NAME}_app.bin
    COMMENT "Generating ${CMAKE_PROJECT_NAME}_app.bin"
)

include(${BOOTLOADER_DIR}/bootloader_target.cmake)
add_stm32f103_bootloader_target(
    ${CMAKE_PROJECT_NAME}_bootloader
    ${CMAKE_CURRENT_SOURCE_DIR}/../mdi
    ${BOOTLOADER_DIR}/STM32F103RB_BOOTLOADER_FLASH.ld
)
configure_stm32f103rb_bootloader_target(${CMAKE_PROJECT_NAME}_bootloader)
