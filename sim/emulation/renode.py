"""Loading Renode into this process (once), and running Monitor commands.

After load() the Renode classes we use are attributes of this module, so every other file does
`from emulation import renode` and calls e.g. `renode.cast(peripheral)`. Before load() they don't exist:
.NET namespaces (Antmicro.*, System) can only be imported after pyrenode3 has loaded Renode.
"""

import os
from pathlib import Path

RENODE_LOG_LEVEL = 3  # 3 = errors only. Renode warns on every unmodelled register bit (very noisy)

# The pinned Renode build, installed by sim/setup_renode.sh
PINNED_RENODE_DIR = Path(__file__).resolve().parent.parent / ".renode"

loaded = False


def load():
    """Load Renode the first time a Board is created.

    pyrenode3 finds Renode through PYRENODE_PATH. If that isn't set, use the pinned build in
    sim/.renode/. Not the `renode` on PATH: the chip model needs that exact build (see
    sim/setup_renode.sh), and another version fails in confusing ways (e.g. no firmware loaded).
    """
    global loaded, monitor, manager, cast, CANMessageFrame, CpuAddressHook
    global Array, Byte, UInt64, Action, Boolean
    if loaded:
        return

    if "PYRENODE_PATH" not in os.environ:
        if not (PINNED_RENODE_DIR / "renode").exists():
            raise RuntimeError(
                "Renode not installed: run sim/setup_renode.sh (or set PYRENODE_PATH to a Renode folder)"
            )
        os.environ["PYRENODE_PATH"] = str(PINNED_RENODE_DIR)

    # These imports load Renode's .NET assemblies; they only work after pyrenode3 is imported.
    from pyrenode3.conversion import interface_to_class
    from pyrenode3.wrappers import Emulation, Monitor
    from Antmicro.Renode.Core import EmulationManager
    from Antmicro.Renode.Core.CAN import CANMessageFrame
    from Antmicro.Renode.Peripherals.CPU import CpuAddressHook
    from System import Action, Array, Boolean, Byte, UInt64

    Emulation()  # creates Renode's emulation (singleton)
    monitor = Monitor()  # runs Monitor commands, like typing them in Renode's window
    manager = EmulationManager.Instance
    # pythonnet returns peripherals as their interface type (IPeripheral, IBusController ...).
    # interface_to_class casts them to the real class, so their own methods/events are visible.
    cast = interface_to_class
    loaded = True


def run(command: str) -> str:
    """Run a Renode Monitor command; raise if Renode reports an error."""
    out, err = monitor.execute(command)
    if err:
        raise RuntimeError(f"Renode command failed: {command}\n{err}")
    return out
