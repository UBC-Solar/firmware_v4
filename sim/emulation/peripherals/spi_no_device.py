# SPI master with nothing on the bus. Renode's F103 model has no SPI at all, so without this every
# HAL_SPI_* call waits for TXE until its timeout (DRD's LCD, MDI's RTD).
#
# Every byte the firmware writes to DR is "sent" instantly: TXE is always set, BSY never, and RXNE
# is set until DR is read. MISO reads 0xFF (an idle line with nothing driving it).
SR, DR = 0x08, 0x0C
RXNE, TXE = 1 << 0, 1 << 1

if request.IsInit:
    regs = {SR: TXE}
    rx_ready = False
elif request.IsRead:
    if request.Offset == SR:
        request.Value = TXE | (RXNE if rx_ready else 0)
    elif request.Offset == DR:
        request.Value = 0xFF
        rx_ready = False
    else:
        request.Value = regs.get(request.Offset, 0)
elif request.IsWrite:
    if request.Offset == DR:
        rx_ready = True
    else:
        regs[request.Offset] = request.Value
