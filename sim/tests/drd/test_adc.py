import pytest

from api import Board

# ADC1 channels
ADC_CH_ACCEL = 8         
ADC_CH_TEMPSENSOR = 16
ADC_CH_VREFINT = 17

DIAGNOSTIC_ID = 0x403     # every 100 ms
MCU_HEALTH_ID = 0x402     # every 1 s

# Distinct sample values per channel
ACCEL_RAW = 2000
VREFINT_RAW = 1489        # -> Vdd 3300 mV
TEMP_RAW = 1775           # -> 25 C
BOOT_MS = 200             # raw_adc1 is 0 until the first drive-state tick


# reads two unsigned bytes
def u16(frame, offset=0):
    return int.from_bytes(frame.data[offset:offset + 2], "little")

# reads two signed bytes
def i16(frame, offset=0):
    return int.from_bytes(frame.data[offset:offset + 2], "little", signed=True)


def vdd_from_raw(raw):
    return 1200 * 4095 // raw


@pytest.fixture
def shared_adc(board: Board):
    """All three consumers of ADC1 get a distinct, known input."""
    board.adc.set(1, ADC_CH_ACCEL,      raw=ACCEL_RAW)
    board.adc.set(1, ADC_CH_VREFINT,    raw=VREFINT_RAW)
    board.adc.set(1, ADC_CH_TEMPSENSOR, raw=TEMP_RAW)
    return board

# test accel
def test_accel_unaffected_by_mcu_health(shared_adc: Board):
    shared_adc.wait(ms=3500)  # several health cycles (1 s each)
    raws = [u16(f) for f in shared_adc.can.sent(DIAGNOSTIC_ID) if f.time_ms >= BOOT_MS]
    assert raws and all(r == ACCEL_RAW for r in raws), sorted(set(raws)) # Asserts non-empty and every frame matches distinct sample value

# test mcu health
def test_mcu_health_unaffected_by_accel(shared_adc: Board):
    shared_adc.wait(ms=3500)
    frames = shared_adc.can.sent(MCU_HEALTH_ID)
    assert frames
    assert all(u16(f, 0) == 3300 for f in frames), [u16(f, 0) for f in frames]
    assert all(i16(f, 2) == 25 for f in frames), [i16(f, 2) for f in frames]

# test both together
def test_inputs_tracked_independently(shared_adc: Board):
    shared_adc.wait(ms=1500)                             # Run 1500ms of simulation
    shared_adc.adc.set(1, ADC_CH_ACCEL, raw=3000)        # Change channel raw value
    shared_adc.adc.set(1, ADC_CH_VREFINT, raw=2482)      # Vdd = 1979 mV
    shared_adc.wait(ms=2500)                             # Run 2500ms of simulation
    assert u16(shared_adc.can.sent(DIAGNOSTIC_ID)[-1]) == 3000  # Validate response
    assert u16(shared_adc.can.sent(MCU_HEALTH_ID)[-1], 0) == vdd_from_raw(2482)