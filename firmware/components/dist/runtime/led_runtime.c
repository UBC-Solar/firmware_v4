#include "led_runtime.h"
#include "i2c.h"
#include "main.h"

IS31FL3236_HandleTypeDef hled;

// Bitmask tracking on/off state per channel: bit n = state of OUTn
static uint32_t led_state = 0;

// All four possible I2C addresses — which one responds depends on how the AD pin is wired.
static const uint8_t kAddrCandidates[] = {
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_GND),
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_VCC),
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_SCL),
    IS31FL3236_GET_I2C_ADDR(IS31FL3236_I2C_AD_TO_SDA),
};

/*============================================================================*/
/* ASYNC I2C WRITE QUEUE
 *
 * HAL_I2C_Master_Transmit_IT starts a transfer and returns immediately. The
 * HAL drives the bus from the I2C EV/ER interrupts and fires
 * HAL_I2C_MasterTxCpltCallback (or HAL_I2C_ErrorCallback) when done.
 *
 * Each queue entry owns its own 2-byte buffer, so the data remains valid in
 * memory until the callback pops the entry — safe to use with IT transfers.
 *
 * channel_toggle pushes a PWM write followed by an UPDATE write, then kicks
 * the queue if the bus is idle. Subsequent entries are chained from the
 * completion/error callbacks.
 */

#define I2C_WQ_SIZE 16U

typedef struct {
    uint8_t buf[2]; // {register_address, value}
} I2C_WriteEntry_t;

static I2C_WriteEntry_t i2c_wq[I2C_WQ_SIZE];
static volatile uint8_t i2c_wq_head = 0;
static volatile uint8_t i2c_wq_tail = 0;
static volatile uint8_t i2c_busy    = 0;

static void i2c_wq_push(uint8_t reg, uint8_t val)
{
    uint8_t next = (i2c_wq_head + 1U) % I2C_WQ_SIZE;
    if (next == i2c_wq_tail) return; // queue full — drop silently
    i2c_wq[i2c_wq_head].buf[0] = reg;
    i2c_wq[i2c_wq_head].buf[1] = val;
    i2c_wq_head = next;
}

static void i2c_start_next(void)
{
    if (i2c_wq_tail == i2c_wq_head) {
        i2c_busy = 0;
        return;
    }
    i2c_busy = 1;
    HAL_I2C_Master_Transmit_IT(
        hled.Init.I2C_Bus,
        hled.Init.I2C_Device_Address,
        i2c_wq[i2c_wq_tail].buf,
        2U
    );
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_wq_tail = (i2c_wq_tail + 1U) % I2C_WQ_SIZE;
    i2c_start_next();
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    // Skip the failed entry so a transient NACK doesn't stall the queue.
    i2c_wq_tail = (i2c_wq_tail + 1U) % I2C_WQ_SIZE;
    i2c_start_next();
}

/*============================================================================*/
/* PUBLIC API */

/**
 * @brief Probe the I2C bus for the IS31FL3236A LED driver and initialise it.
 *
 * The peripheral is reinitialised between attempts to recover from any stuck
 * clock or bus state left by a failed probe. Returns as soon as the first
 * responsive address is found.
 *
 * @return 1 if the device was found and initialised, 0 if no device responded.
 */
uint8_t LED_Driver_Init(void)
{
    HAL_GPIO_WritePin(LED_TOGGLE_GPIO_Port, LED_TOGGLE_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    for (int attempt = 0; attempt < 3; attempt++)
    {
        HAL_I2C_DeInit(&hi2c1);
        HAL_I2C_Init(&hi2c1);

        for (int i = 0; i < 4; i++)
        {
            if (HAL_I2C_IsDeviceReady(&hi2c1, kAddrCandidates[i], 1, 10) == HAL_OK)
            {
                hled.Init.I2C_Bus                           = &hi2c1;
                hled.Init.I2C_Device_Address                = kAddrCandidates[i];
                hled.Init.I2C_Transmit_Timeout_Milliseconds = 100;
                hled.Init.Chip_Enable_Signal_Port           = LED_TOGGLE_GPIO_Port;
                hled.Init.Chip_Enable_Signal_Pin            = LED_TOGGLE_Pin;
                IS31FL3236A_Init();
                return 1;
            }
        }
    }
    return 0;
}

void IS31FL3236A_Init(void)
{
    HAL_Delay(10); // allow chip to stabilise after power-on

    IS31FL3236_Init(&hled);

    // 3kHz PWM frequency keeps switching noise out of the audible range
    IS31FL3236_WriteRegister(&hled, 0x4B, 0x00);

    // All channels on at max current; individual PWM values control brightness
    IS31FL3236_WriteGlobalLEDControl(&hled, IS31FL3236_LED_CURRENT_MAX, IS31FL3236_LED_STATE_ON);
    IS31FL3236_Update(&hled);
}

/**
 * @brief Toggle a single LED channel and queue the resulting I2C writes.
 *
 * Two writes are queued per call: one to the PWM register and one UPDATE
 * command to latch the new value. Both are sent asynchronously from the I2C
 * interrupt context via the async write queue above.
 *
 * @param out_num Physical OUT pin number (1-indexed, matching IS31FL3236A silk).
 */
static void channel_toggle(uint8_t out_num)
{
    led_state ^= (1U << out_num);
    uint8_t pwm = ((led_state >> out_num) & 1U) ? 0xFF : 0x00;

    // Library channels are 0-indexed; OUTn maps to channel (n-1)
    i2c_wq_push(IS31FL3236_REGISTER_PWM + (out_num - 1U), pwm);
    i2c_wq_push(IS31FL3236_REGISTER_UPDATE, 0x00);

    if (!i2c_busy) {
        i2c_start_next();
    }
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
