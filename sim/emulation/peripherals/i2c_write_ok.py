# I2C master where every write succeeds, for boards that only write to their I2C devices (STR's
# AS1115 hex display driver). Replaces Renode's STM32F1_I2C, which never finishes a polled HAL
# write: with no device the address isn't acked, and with Mocks.DummyI2CSlave BTF never gets set,
# so HAL_I2C_Master_Transmit(..., HAL_MAX_DELAY) waits forever.
#
# HAL's polled write waits for these SR1 flags in turn; reporting them all as set means each wait
# passes immediately. Writes to CR1/CR2/DR/... just stick. Reads from a device would get garbage,
# so don't use this for boards that read over I2C.
SR1, SR2 = 0x14, 0x18
SB, ADDR, BTF, TXE = 1 << 0, 1 << 1, 1 << 2, 1 << 7   # start sent, address acked, byte done, DR empty
MSL = 1 << 0                                         # SR2: master mode; BUSY (bit 1) stays 0

if request.IsInit:
    regs = {}
elif request.IsRead:
    if request.Offset == SR1:
        request.Value = SB | ADDR | BTF | TXE
    elif request.Offset == SR2:
        request.Value = MSL
    else:
        request.Value = regs.get(request.Offset, 0)
elif request.IsWrite:
    regs[request.Offset] = request.Value
