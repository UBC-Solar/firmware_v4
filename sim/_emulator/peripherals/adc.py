# Minimal STM32F1 ADC stub: registers stick, CR2.CAL/RSTCAL self-clear (calibration "finishes"
# instantly), SR always reports end-of-conversion, and DR reads a fixed mid-scale value.
SR, CR2, DR = 0x00, 0x08, 0x4C

if request.IsInit:
    regs = {}
elif request.IsRead:
    if request.Offset == SR:
        request.Value = 0x12            # EOC | STRT
    elif request.Offset == DR:
        request.Value = 0x800           # 12-bit mid-scale
    else:
        request.Value = regs.get(request.Offset, 0)
elif request.IsWrite:
    v = request.Value
    if request.Offset == CR2:
        v &= ~0xC                       # CAL (bit 2), RSTCAL (bit 3) complete immediately
    regs[request.Offset] = v
