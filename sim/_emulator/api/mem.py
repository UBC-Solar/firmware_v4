"""board.mem: read and write firmware variables by name.

Whole variables only (looked up in the ELF's symbol table). For a struct field, use read_bytes()
with the field's offset; reading fields by name would need DWARF (pyelftools), not done yet.
Read while the emulation is paused (between run_for() calls), so values don't change mid-read.
"""

from . import _renode

MAX_INT_SIZE = 8


class Mem:
    def __init__(self, board):
        self._board = board

    def _symbol_info(self, name: str) -> tuple[int, int]:
        """(address, size in bytes) of a variable."""
        board = self._board
        address = board._address_of(name)
        if address is None:
            raise ValueError(f"No symbol '{name}' in {board.elf.name}")
        # Out-parameters (name, symbol) are passed as None and come back in the result tuple
        found, _, symbol = board._bus.TryFindSymbolAt(_renode.UInt64(address), None, None,
                                                       board._cpu, False)
        if not found or symbol is None:
            raise ValueError(f"Can't find the size of '{name}' at 0x{address:08x}")
        return address, int(symbol.Length.RawValue)

    def read_bytes(self, name: str, offset: int = 0, size: int | None = None) -> bytes:
        """Raw bytes of a variable (default: all of it), optionally from `offset`."""
        address, length = self._symbol_info(name)
        if size is None:
            size = length - offset
        if offset < 0 or size < 0 or offset + size > length:
            raise ValueError(f"'{name}' is {length} bytes; can't read {size} at offset {offset}")
        return bytes(self._board._bus.ReadBytes(_renode.UInt64(address + offset), size))

    def read(self, name: str, size: int | None = None, signed: bool = False) -> int:
        """A variable as a little-endian integer, e.g. read("g_time_since_bootup").

        `size` reads only the first bytes. Variables over 8 bytes (structs, arrays) need size or
        read_bytes().
        """
        _, length = self._symbol_info(name)
        size = length if size is None else size
        if size > MAX_INT_SIZE:
            raise ValueError(f"'{name}' is {length} bytes, too big for an int: use "
                             f"mem.read_bytes('{name}') or pass size=")
        return int.from_bytes(self.read_bytes(name, 0, size), "little", signed=signed)

    def write(self, name: str, value: int, size: int | None = None):
        """Overwrite a variable with a little-endian integer (fault injection, forcing a state)."""
        address, length = self._symbol_info(name)
        size = length if size is None else size
        if size > length:
            raise ValueError(f"'{name}' is {length} bytes; can't write {size}")
        data = value.to_bytes(size, "little", signed=value < 0)
        self._board._announce(f"mem.write {name} = {value} ({data.hex(' ')})")
        r = _renode
        self._board._bus.WriteBytes(r.Array[r.Byte](data), r.UInt64(address), False, self._board._cpu)
