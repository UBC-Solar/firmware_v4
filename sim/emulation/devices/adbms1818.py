# SPI2 with MST's chain of ADBMS1818 battery monitors behind it (driver/spi_driver.c).
# Replaces the SPI peripheral itself (PyDev on SPI2's registers): every byte the firmware writes
# to DR is one byte clocked out on MOSI, and the next DR read is what the chain sent back on MISO.
#
# Protocol (ADBMS1818 datasheet): with CS low the firmware sends a 2-byte command + 2-byte PEC.
#   WR... commands: then 8 bytes (6 data + 2 PEC) per device, last device first.  Stored.
#   RD... commands: then the chain answers 8 bytes per device, first device first.
#   anything else (ADCV, ADAX, PLADC ...): conversions finish instantly; MISO reads 0xFF, so
#     polling (PLADC) sees "done" right away.
# CS is a GPIO this script can't see, so commands are found in the byte stream instead: the last
# 4 bytes written form a command when their PEC is valid. The zero bytes the firmware clocks out
# after releasing CS never form a valid PEC, so they can't be mistaken for a command.
#
# What the chain measures (fixed for now):
#   every cell 3.6 V                  (cell registers, 100 uV per bit)
#   every thermistor 1.5 V = 25 degC  (aux registers; 10k NTC with a 10k pull-up from 3 V)
SR, DR = 0x08, 0x0C
RXNE, TXE = 1 << 0, 1 << 1
NUM_DEVICES = 2                      # SLAVE_NUM_DEVICES in mst/app/mst_defs.h
CELL_100UV = 36000
AUX_100UV = 15000

READ_GROUPS = {0x0004, 0x0006, 0x0008, 0x000A, 0x0009, 0x000B}   # RDCVA..F
AUX_GROUPS = {0x000C, 0x000E, 0x000D, 0x000F}                    # RDAUXA..D
READ_OF = {0x0001: 0x0002, 0x0024: 0x0026}                       # WRCFGA/B -> RDCFGA/B


def pec15(data):
    remainder = 16
    for byte in data:
        address = ((remainder >> 7) ^ byte) & 0xFF
        remainder = ((remainder << 8) & 0xFFFF) ^ PEC_TABLE[address]
    return (remainder << 1) & 0xFFFF


def make_pec_table():
    table = []
    for i in range(256):
        remainder = i << 7
        for _ in range(8):
            remainder = ((remainder << 1) ^ 0x4599) if remainder & 0x4000 else (remainder << 1)
        table.append(remainder & 0xFFFF)
    return table


def with_pec(group):
    pec = pec15(group)
    return group + [pec >> 8, pec & 0xFF]


def group_value(value):
    lo, hi = value & 0xFF, value >> 8
    return [lo, hi, lo, hi, lo, hi]


def answer(command):
    """Bytes the chain sends back for a read command, first device first."""
    out = []
    for device in range(NUM_DEVICES):
        if command in READ_GROUPS:
            group = group_value(CELL_100UV)
        elif command in AUX_GROUPS:
            group = group_value(AUX_100UV)
        else:  # config / other register groups: what was last written, else zeros
            group = stored.get((command, device), [0] * 6)
        out += with_pec(group)
    return out


def on_byte(byte):
    """One byte clocked out by the firmware; returns the byte clocked in."""
    global mode, pending, written, window
    if mode == "write":                 # data of a WR command
        written.append(byte)
        if len(written) == 8 * NUM_DEVICES:
            for i in range(NUM_DEVICES):
                device = NUM_DEVICES - 1 - i          # writes go last device first
                stored[(READ_OF[write_command], device)] = written[8 * i: 8 * i + 6]
            mode = "command"
        return 0xFF
    if mode == "read":
        reply = pending.pop(0)
        if not pending:
            mode = "command"
        return reply

    window = (window + [byte])[-4:]
    if len(window) == 4 and pec15(window[:2]) == (window[2] << 8 | window[3]):
        start_command(window[0] << 8 | window[1])
        window = []
    return 0xFF


def start_command(command):
    global mode, pending, written, write_command
    if command in READ_GROUPS or command in AUX_GROUPS or command in READ_OF.values():
        mode, pending = "read", answer(command)
    elif command in READ_OF:
        mode, written, write_command = "write", [], command
    # everything else (conversions, PLADC, ...) completes instantly


if request.IsInit:
    PEC_TABLE = make_pec_table()
    regs = {}
    stored = {}            # (read command, device) -> 6 bytes
    mode, pending, written, window, write_command = "command", [], [], [], 0
    rx_byte, rx_ready = 0xFF, False
elif request.IsRead:
    if request.Offset == SR:
        request.Value = TXE | (RXNE if rx_ready else 0)
    elif request.Offset == DR:
        request.Value = rx_byte
        rx_ready = False
    else:
        request.Value = regs.get(request.Offset, 0)
elif request.IsWrite:
    if request.Offset == DR:
        rx_byte, rx_ready = on_byte(request.Value & 0xFF), True
    else:
        regs[request.Offset] = request.Value
