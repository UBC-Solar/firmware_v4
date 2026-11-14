# Minimal STM32F1 ADC stub: registers stick, CR2.CAL/RSTCAL self-clear (calibration "finishes"
# instantly), SR always reports end-of-conversion, and DR reads the value stored for the channel
# being converted (default 0x800, mid-scale).
#
# Per-channel values live in a test "backdoor" at 0x100 + 4 * channel (channels 0-17). Real hardware
# has nothing there, so firmware never touches it; board.adc.set() writes it through sysbus.
# The converted channel is the first one in the regular sequence: SQR3 bits 4:0, which is what
# HAL_ADC_ConfigChannel(..., Rank 1) sets. Scan mode (several channels, DMA) isn't modelled.
SR, CR2, SQR3, DR = 0x00, 0x08, 0x34, 0x4C
BACKDOOR = 0x100

if request.IsInit:
    regs = {}
elif request.IsRead:
    if request.Offset == SR:
        request.Value = 0x12            # EOC | STRT
    elif request.Offset == DR:
        channel = regs.get(SQR3, 0) & 0x1F
        request.Value = regs.get(BACKDOOR + 4 * channel, 0x800)
    else:
        request.Value = regs.get(request.Offset, 0)
elif request.IsWrite:
    v = request.Value
    if request.Offset == CR2:
        v &= ~0xC                       # CAL (bit 2), RSTCAL (bit 3) complete immediately
    regs[request.Offset] = v
