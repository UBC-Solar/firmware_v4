#include "i2c_driver.h"
#include "i2c.h"

// 7-bit address with AD tied to GND (AD=00 -> 0x3C), shifted for HAL
#define IS31FL3236A_ADDR        (0x3C << 1)

#define REG_SHUTDOWN            0x00
#define REG_UPDATE              0x25
#define REG_FREQ                0x4B
#define REG_RESET               0x4F

// PWM register for OUTn = n  (e.g. OUT3 -> 0x03)
#define PWM_REG(n)              (n)
// LED Control register for OUTn = 0x25 + n  (e.g. OUT3 -> 0x28)
#define LED_CTRL_REG(n)         (0x25 + (n))

// Bitmask tracking on/off state per channel: bit n = state of OUTn
static uint32_t led_state = 0;

static void write_reg(uint8_t reg, uint8_t value) {
    HAL_I2C_Mem_Write(&hi2c1, IS31FL3236A_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, HAL_MAX_DELAY);
}

static void channel_toggle(uint8_t out_num) {
    uint8_t pwm;

    led_state ^= (1U << out_num);

    if ((led_state >> out_num) & 1U) {
        pwm = 0xFF;
    } else {
        pwm = 0x00;
    }

    write_reg(PWM_REG(out_num), pwm);
    write_reg(REG_UPDATE, 0x00);
}

void IS31FL3236A_Init(void) {
    // Enable OUT3~OUT21 using auto-increment (LED ctrl regs 0x28~0x3A = 19 bytes)
    uint8_t buf[19];
    for (int i = 0; i < 19; i++) buf[i] = 0x01;

    write_reg(REG_RESET, 0x00);    // reset all registers to default
    write_reg(REG_SHUTDOWN, 0x01); // SSD=1, normal operation
    write_reg(REG_FREQ, 0x00);     // OFS=0, 3kHz PWM (audio range)

    HAL_I2C_Mem_Write(&hi2c1, IS31FL3236A_ADDR, LED_CTRL_REG(3),
                      I2C_MEMADD_SIZE_8BIT, buf, 19, HAL_MAX_DELAY);

    write_reg(REG_UPDATE, 0x00);   // latch LED control registers
}

void IS31FL3236A_HLIM_Toggle(void)        { channel_toggle(3);  }
void IS31FL3236A_LLIM_Toggle(void)        { channel_toggle(4);  }
void IS31FL3236A_NEG_Toggle(void)         { channel_toggle(5);  }
void IS31FL3236A_DIST_FAULT_Toggle(void)  { channel_toggle(6);  }
void IS31FL3236A_ESTOP_Toggle(void)       { channel_toggle(7);  }
void IS31FL3236A_CAN_FAULT_Toggle(void)   { channel_toggle(8);  }
void IS31FL3236A_POS_Toggle(void)         { channel_toggle(9);  }
void IS31FL3236A_MOTOR_PC_Toggle(void)    { channel_toggle(10); }
void IS31FL3236A_MPPT_PC_Toggle(void)     { channel_toggle(11); }
void IS31FL3236A_IMD_Toggle(void)         { channel_toggle(12); }
void IS31FL3236A_MPPT_Toggle(void)        { channel_toggle(13); }
void IS31FL3236A_DCH_ON_Toggle(void)      { channel_toggle(14); }
void IS31FL3236A_DCH_OFF_Toggle(void)     { channel_toggle(15); }
void IS31FL3236A_FANS_Toggle(void)        { channel_toggle(17); }
void IS31FL3236A_SUPP_ACTIVE_Toggle(void) { channel_toggle(18); }
void IS31FL3236A_SUPP_LOW_Toggle(void)    { channel_toggle(19); }
void IS31FL3236A_DCDC_ACTIVE_Toggle(void) { channel_toggle(20); }
void IS31FL3236A_CONTACTOR_Toggle(void)   { channel_toggle(21); }
