# Minimal STM32F1 RTC stub: registers stick; CRL always reports RTOFF (last write done) and
# RSF (registers synchronized) so HAL_RTC_Init / WaitForSynchro don't time out.
# The counter does not tick on its own.
CRL = 0x04

if request.IsInit:
    regs = {}
elif request.IsRead:
    v = regs.get(request.Offset, 0)
    if request.Offset == CRL:
        v |= 0x28                       # RTOFF (bit 5) | RSF (bit 3)
    request.Value = v
elif request.IsWrite:
    regs[request.Offset] = request.Value
