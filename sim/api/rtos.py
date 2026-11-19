"""board.rtos: what the FreeRTOS tasks are doing."""

RTOS_WAIT_FUNCTIONS = (
    "osDelay", "osDelayUntil", "osMessageQueueGet", "osSemaphoreAcquire",
    "osThreadFlagsWait", "osEventFlagsWait",
)


class Rtos:
    def __init__(self, board, trace_loops: bool = False):
        self._board = board
        self._tracing = trace_loops
        self._tasks_created = []
        self._wait_calls = {}  # calling function -> number of calls to an RTOS wait function

        def on_thread_new():
            # osThreadNew(func, ...): R0 holds the task's function
            self._tasks_created.append(board._symbol(board._cpu.GetRegister(0).RawValue))

        def on_wait():
            # LR is the return address, i.e. inside the function that called the wait
            caller = board._symbol(board._cpu.LR.RawValue)
            self._wait_calls[caller] = self._wait_calls.get(caller, 0) + 1

        # osThreadNew runs a handful of times at boot: always cheap enough to hook
        if not board._add_hook("osThreadNew", on_thread_new):
            return  # bare-metal board, no RTOS
        # The waits run ~1000x per firmware second, and each CPU address hook costs a few ms of
        # wall time, so they're only hooked when a test asks (@pytest.mark.rtos_trace)
        if not trace_loops:
            return
        for wait in RTOS_WAIT_FUNCTIONS:
            board._add_hook(wait, on_wait)

    def task_loops(self) -> dict[str, int]:
        """Loop count of every task created with osThreadNew, by task function name.

        Counted by hooking osThreadNew (records each task function) and the RTOS wait functions
        (osDelay, osMessageQueueGet, osSemaphoreAcquire, ...: one call from a task = one loop).
        A task that was never created, hangs or starves shows up with a low count. Only counts
        waits called directly from the task function, not from helpers it calls.

        Needs the test marked @pytest.mark.rtos_trace (it slows the emulation several times over).
        """
        if not self._tracing:
            raise RuntimeError(
                "rtos.task_loops() needs loop tracing: mark the test @pytest.mark.rtos_trace"
            )
        return {task: self._wait_calls.get(task, 0) for task in self._tasks_created}
