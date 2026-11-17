"""board.adc: set what the ADC reads on each channel.

Works with peripherals/adc.py, which returns the value stored for the channel being converted. The
values sit in a "backdoor" at offset 0x100 + 4 * channel of each ADC: real hardware has nothing
there, so the firmware never touches it.

Supported: single conversions of one channel (HAL_ADC_ConfigChannel + Start + PollForConversion +
GetValue), which every board except HVC uses. Not yet: scan mode + DMA (HVC).
"""

ADC_BASES = {1: 0x40012400, 2: 0x40012800}
BACKDOOR = 0x100
CHANNELS = 18  # 0-15 external pins, 16 temperature sensor, 17 VREFINT
VREF_VOLTS = 3.3
FULL_SCALE = 4095


class Adc:
    def __init__(self, board):
        self._board = board

    def set(self, adc: int, channel: int, volts: float | None = None, raw: int | None = None):
        """Make ADC `adc` (1 or 2) read `channel` as `volts` (0-3.3 V) or as `raw` (0-4095).

        E.g. set(1, 17, raw=1489) makes STR's VREFINT read 1489, so it computes Vdd = 3300 mV.
        Channels not set read 0x800 (mid-scale).
        """
        if adc not in ADC_BASES:
            raise ValueError(f"No ADC{adc}. Known: {', '.join(map(str, ADC_BASES))}")
        if not 0 <= channel < CHANNELS:
            raise ValueError(f"ADC channel {channel} out of range 0-{CHANNELS - 1}")
        if (volts is None) == (raw is None):
            raise ValueError("Give exactly one of volts= or raw=")
        if raw is None:
            raw = round(volts / VREF_VOLTS * FULL_SCALE)
        raw = max(0, min(FULL_SCALE, int(raw)))
        self._board._announce(f"adc.set ADC{adc} ch{channel} = {raw}"
                              + (f" ({volts} V)" if volts is not None else ""))
        self._board._bus.WriteDoubleWord(ADC_BASES[adc] + BACKDOOR + 4 * channel, raw)
