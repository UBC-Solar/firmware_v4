from _emulator import Board

STR_CAN_MSG_ID = 0x580  # driver/can_driver.h, sent by TasksSteeringOutputs
STEERING_TASK_DELAY_MS = 100  # runtime/tasks.c


def test_steering_state_sent_every_100ms(board: Board):
    board.run_for(ms=1000)
    frames = board.can_frames_sent(can_id=STR_CAN_MSG_ID)
    gaps = [b.time_ms - a.time_ms for a, b in zip(frames, frames[1:])]
    assert len(frames) >= 8, frames
    assert all(90 <= g <= 110 for g in gaps), gaps
