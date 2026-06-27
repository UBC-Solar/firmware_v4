#ifndef _IS31FL3236_DRIVER_H_
#define _IS31FL3236_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stm32f1xx_hal.h>

#define IS31FL3236_I2C_AD_TO_GND                0x00
#define IS31FL3236_I2C_AD_TO_SCL                0x02
#define IS31FL3236_I2C_AD_TO_SDA                0x04
#define IS31FL3236_I2C_AD_TO_VCC                0x06
#define IS31FL3236_GET_I2C_ADDR(AD_conn)        (0x78 | AD_conn)

#define IS31FL3236_MAX_CHANNELS                 36
#define IS31FL3236_MAX_RGB_CHANNELS             (IS31FL3236_MAX_CHANNELS / 3)

#define IS31FL3236_REGISTER_SHUTDOWN            0x00
#define IS31FL3236_REGISTER_PWM                 0x01
#define IS31FL3236_REGISTER_LED_CTRL            0x26
#define IS31FL3236_REGISTER_UPDATE              0x25
#define IS31FL3236_REGISTER_GLOBAL_CTRL         0x4a
#define IS31FL3236_REGISTER_RESET               0x4f

#define IS31FL3236_LED_CURRENT_MAX              0x00
#define IS31FL3236_LED_CURRENT_MAX_DIV_2        0x02
#define IS31FL3236_LED_CURRENT_MAX_DIV_3        0x04
#define IS31FL3236_LED_CURRENT_MAX_DIV_4        0x06

#define IS31FL3236_LED_STATE_OFF                0x00
#define IS31FL3236_LED_STATE_ON                 0x01

#define IS31FL3236_SOFTWARE_SHUTDOWN_ENABLED    0x00
#define IS31FL3236_SOFTWARE_SHUTDOWN_DISABLED   0x01

#define IS31FL3236_CHIP_DISABLED                GPIO_PIN_RESET
#define IS31FL3236_CHIP_ENABLED                 GPIO_PIN_SET

typedef struct
{
    I2C_HandleTypeDef* I2C_Bus;
    uint8_t I2C_Device_Address;
    uint32_t I2C_Transmit_Timeout_Milliseconds;
    GPIO_TypeDef* Chip_Enable_Signal_Port;
    uint16_t Chip_Enable_Signal_Pin;
    uint8_t RGB_Mode_Color_1;
    uint8_t RGB_Mode_Color_2;
    uint8_t RGB_Mode_Color_3;
} IS31FL3236_InitTypeDef;

typedef struct
{
    IS31FL3236_InitTypeDef Init;
} IS31FL3236_HandleTypeDef;

void IS31FL3236_Init(IS31FL3236_HandleTypeDef* handle);
void IS31FL3236_SetChipEnable(IS31FL3236_HandleTypeDef* handle, uint8_t enable_state);
HAL_StatusTypeDef IS31FL3236_WriteRegister(IS31FL3236_HandleTypeDef* handle, uint8_t register_address, uint8_t value);
void IS31FL3236_Reset(IS31FL3236_HandleTypeDef* handle);
void IS31FL3236_Update(IS31FL3236_HandleTypeDef* handle);
void IS31FL3236_WriteLEDControl(IS31FL3236_HandleTypeDef* handle, uint8_t channel, uint8_t led_current_setting, uint8_t led_state);
void IS31FL3236_WriteGlobalLEDControl(IS31FL3236_HandleTypeDef* handle, uint8_t led_current_setting, uint8_t led_state);
void IS31FL3236_WritePWM(IS31FL3236_HandleTypeDef* handle, uint8_t channel, uint8_t pwm_value);
void IS31FL3236_SetSoftwareShutdown(IS31FL3236_HandleTypeDef* handle, uint8_t software_shutdown_mode);

#ifdef __cplusplus
}
#endif

#endif
