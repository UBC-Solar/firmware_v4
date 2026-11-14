from _emulator import Board

STR_DIAGNOSTICS_CAN_ID = 0x591  # driver/can_driver.h: [Vdd mV LE][temp C LE][TEC][REC], every 100 ms
STR_FLAGS_CAN_ID = 0x592  # [flags]: bit 0 mcu_under_voltage, bit 1 mcu_over_temp
MCU_UNDER_VOLTAGE = 0x01
ADC_CHANNEL_VREFINT = 17  # Vdd_mV = 4095 * 1200 / raw (driver/mcu_health_driver.c)


def vdd_mv(frame) -> int:
    return int.from_bytes(frame.data[0:2], "little")


def test_vdd_reported_from_vrefint(board: Board):
    board.adc.set(1, ADC_CHANNEL_VREFINT, raw=1489)  # 4095 * 1200 / 1489 = 3300 mV
    board.run_for(ms=300)
    frame = board.can.wait_for(STR_DIAGNOSTICS_CAN_ID, within_ms=200)
    assert vdd_mv(frame) == 3300, frame


def test_low_vdd_sets_under_voltage_flag(board: Board):
    board.adc.set(1, ADC_CHANNEL_VREFINT, volts=2.0)  # raw 2482 -> Vdd 1979 mV, below 2400
    board.run_for(ms=300)
    assert vdd_mv(board.can.wait_for(STR_DIAGNOSTICS_CAN_ID, within_ms=200)) < 2400
    flags = board.can.wait_for(STR_FLAGS_CAN_ID, within_ms=200).data[0]
    assert flags & MCU_UNDER_VOLTAGE, f"flags 0x{flags:02x}"
