import pytest

from api import Board

# ADC1 channels
ADC_CH_ACCEL = 8          # cube/Core/Src/adc.c (MX_ADC1_Init)
ADC_CH_TEMPSENSOR = 16
ADC_CH_VREFINT = 17

DIAGNOSTIC_ID = 0x403     # [raw_adc1 LE][raw_adc2 LE]..., every 100 ms
MCU_HEALTH_ID = 0x402     # [Vdd mV LE][temp C LE][TEC][REC], every 1 s

# Distinct values per channel, so a wrong-channel read is obvious
ACCEL_RAW = 2000
VREFINT_RAW = 1489        # -> Vdd 3300 mV
TEMP_RAW = 1775           # -> 25 C
BOOT_MS = 200             # raw_adc1 is 0 until the first drive-state tick


def u16(frame, offset=0):
    return int.from_bytes(frame.data[offset:offset + 2], "little")


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


def test_both_consumers_are_running(shared_adc: Board):
    # Guards the other tests: with MCU_HEALTH commented out in tasks.c they'd pass vacuously
    shared_adc.wait(ms=3500)
    assert len(shared_adc.can.sent(MCU_HEALTH_ID)) >= 3
    assert len(shared_adc.can.sent(DIAGNOSTIC_ID)) >= 30

def test_debug_health_task(board: Board):
    board.wait(ms=3500)
    print("0x404 uptime frames:", len(board.can.sent(0x404)))
    print("0x405 hash frames:  ", len(board.can.sent(0x405)))
    print("0x402 health frames:", len(board.can.sent(0x402)))
    print("PC at end:", board.symbol_at_pc())
    print("task loops:", board.rtos.task_loops())

def test_accel_unaffected_by_mcu_health(shared_adc: Board):
    shared_adc.wait(ms=3500)  # several health cycles (1 s each)
    raws = [u16(f) for f in shared_adc.can.sent(DIAGNOSTIC_ID) if f.time_ms >= BOOT_MS]
    # Every frame, not just the last: catches a read that is only wrong some of the time
    assert raws and all(r == ACCEL_RAW for r in raws), sorted(set(raws))


def test_mcu_health_unaffected_by_accel(shared_adc: Board):
    shared_adc.wait(ms=3500)
    frames = shared_adc.can.sent(MCU_HEALTH_ID)
    assert frames
    assert all(u16(f, 0) == 3300 for f in frames), [u16(f, 0) for f in frames]
    assert all(i16(f, 2) == 25 for f in frames), [i16(f, 2) for f in frames]


def test_inputs_tracked_independently(shared_adc: Board):
    shared_adc.wait(ms=1500)
    shared_adc.adc.set(1, ADC_CH_ACCEL, raw=3000)
    shared_adc.adc.set(1, ADC_CH_VREFINT, raw=2482)  # Vdd = 1979 mV
    shared_adc.wait(ms=2500)
    assert u16(shared_adc.can.sent(DIAGNOSTIC_ID)[-1]) == 3000
    assert u16(shared_adc.can.sent(MCU_HEALTH_ID)[-1], 0) == vdd_from_raw(2482)