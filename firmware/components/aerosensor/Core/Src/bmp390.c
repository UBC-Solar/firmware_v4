#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include <stdint.h>
#include "math.h"

/* ================= CONFIG ================= */
#define BMP390_ADDR        (0x76 << 1)

#define REG_DATA           0x04
#define REG_CALIB          0x31
#define REG_PWR_CTRL       0x1B

extern I2C_HandleTypeDef hi2c1;

/* ================= STRUCTS ================= */
typedef struct {
    uint32_t pressure;
    uint32_t temperature;
} bmp3_uncomp_data;

typedef struct {
    double par_t1, par_t2, par_t3;
    double par_p1, par_p2, par_p3, par_p4;
    double par_p5, par_p6, par_p7, par_p8;
    double par_p9, par_p10, par_p11;
    double t_lin;
} bmp3_calib_data;

/* ================= GLOBAL ================= */
static bmp3_calib_data calib;

/* ================= INTERNAL ================= */
static void parse_raw(uint8_t *buf, bmp3_uncomp_data *u)
{
    u->pressure =
        ((uint32_t)buf[2] << 16) |
        ((uint32_t)buf[1] << 8)  |
        ((uint32_t)buf[0]);

    u->temperature =
        ((uint32_t)buf[5] << 16) |
        ((uint32_t)buf[4] << 8)  |
        ((uint32_t)buf[3]);
}

static void parse_calib(uint8_t *reg)
{
    int16_t par_t1 = (reg[1] << 8) | reg[0];
    int16_t par_t2 = (reg[3] << 8) | reg[2];
    int8_t  par_t3 = reg[4];

    int16_t par_p1 = (reg[6] << 8) | reg[5];
    int16_t par_p2 = (reg[8] << 8) | reg[7];
    int8_t  par_p3 = reg[9];
    int8_t  par_p4 = reg[10];
    uint16_t par_p5 = (reg[12] << 8) | reg[11];
    uint16_t par_p6 = (reg[14] << 8) | reg[13];
    int8_t  par_p7 = reg[15];
    int8_t  par_p8 = reg[16];
    int16_t par_p9 = (reg[18] << 8) | reg[17];
    int8_t  par_p10 = reg[19];
    int8_t  par_p11 = reg[20];

    calib.par_t1 = par_t1 / 0.00390625;
    calib.par_t2 = par_t2 / 1073741824.0;
    calib.par_t3 = par_t3 / 281474976710656.0;

    calib.par_p1 = (par_p1 - 16384) / 1048576.0;
    calib.par_p2 = (par_p2 - 16384) / 536870912.0;
    calib.par_p3 = par_p3 / 4294967296.0;
    calib.par_p4 = par_p4 / 137438953472.0;
    calib.par_p5 = par_p5 / 0.125;
    calib.par_p6 = par_p6 / 64.0;
    calib.par_p7 = par_p7 / 256.0;
    calib.par_p8 = par_p8 / 32768.0;
    calib.par_p9 = par_p9 / 281474976710656.0;
    calib.par_p10 = par_p10 / 281474976710656.0;
    calib.par_p11 = par_p11 / 36893488147419103232.0;
}

static double comp_temp(bmp3_uncomp_data *u)
{
    double p1 = (double)u->temperature - calib.par_t1;
    double p2 = p1 * calib.par_t2;

    calib.t_lin = p2 + (p1 * p1) * calib.par_t3;
    return calib.t_lin;
}

static double comp_press(bmp3_uncomp_data *u)
{
    double t = calib.t_lin;
    double p = u->pressure;

    double offset =
        calib.par_p5 +
        calib.par_p6 * t +
        calib.par_p7 * t * t +
        calib.par_p8 * t * t * t;

    double sensitivity =
        calib.par_p1 +
        calib.par_p2 * t +
        calib.par_p3 * t * t +
        calib.par_p4 * t * t * t;

    double pressure =
        offset +
        p * sensitivity +
        (p * p) * (calib.par_p9 + calib.par_p10 * t) +
        (p * p * p) * calib.par_p11;

    return pressure;
}

/* ================= PUBLIC API ================= */

HAL_StatusTypeDef BMP390_Init(void)
{
    uint8_t calib_buf[21];
    uint8_t pwr = 0x33;

    // Read calibration
    if (HAL_I2C_Mem_Read(&hi2c1, BMP390_ADDR,
                         REG_CALIB, I2C_MEMADD_SIZE_8BIT,
                         calib_buf, 21, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;

    parse_calib(calib_buf);

    // Enable sensor (temp + press, normal mode)
    if (HAL_I2C_Mem_Write(&hi2c1, BMP390_ADDR,
                          REG_PWR_CTRL, I2C_MEMADD_SIZE_8BIT,
                          &pwr, 1, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef BMP390_Read(volatile float *temp_c, volatile float *press_pa)
{
    uint8_t buf[6];
    bmp3_uncomp_data u;

    if (HAL_I2C_Mem_Read(&hi2c1, BMP390_ADDR,
                         REG_DATA, I2C_MEMADD_SIZE_8BIT,
                         buf, 6, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;

    parse_raw(buf, &u);

    double t = comp_temp(&u);
    double p = comp_press(&u);

    *temp_c = (float)t;
    *press_pa = (float)p;

    return HAL_OK;
}

float BMP390_ReadAltitude(volatile float pressure_pa)
{
    const float P0 = 102260.0f; // sea level pressure (Pa)

    return 44330.0f * (1.0f - powf(pressure_pa / P0, 0.1903f));
}