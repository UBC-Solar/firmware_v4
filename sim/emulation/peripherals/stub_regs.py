# Plain read/write register storage for peripherals Renode doesn't model (values just stick).
if request.IsInit:
    regs = {}
elif request.IsRead:
    request.Value = regs.get(request.Offset, 0)
elif request.IsWrite:
    regs[request.Offset] = request.Value
