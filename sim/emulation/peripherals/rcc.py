CR, CFGR, CSR = 0x00, 0x04, 0x24

def mirror(offset, v):
    if offset == CR:
        v &= ~(1 << 1) & ~(1 << 17) & ~(1 << 25)
        v |= (v & 1) << 1
        v |= (v & (1 << 16)) << 1
        v |= (v & (1 << 24)) << 1
        pass
    elif offset == CFGR:
        v &= ~(3 << 2)
        v |= (v & 3) << 2
        pass
    elif offset == CSR:
        v &= ~(1 << 1)
        v |= (v & 1) << 1
        pass
    return v

# Plain register storage: writes stick, reads return the last write (0 if never written).
if request.IsInit:
    regs = {CR: 0x83, CSR: 0x0C000000}      # reset values
elif request.IsWrite:
    regs[request.Offset] = mirror(request.Offset, request.Value)
elif request.IsRead:
    request.Value = regs.get(request.Offset, 0)