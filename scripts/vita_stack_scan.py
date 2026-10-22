#!/usr/bin/env python3
#
# Lists the likely call chain of the crashed (or any) thread in a Vita core dump by scanning its stack for return
# addresses. vita-core-dump's unwinder stops at the first frame without CFI, which is every frame in newlib, vitaGL
# and the shader compiler, so this is the fallback: every Thumb code pointer into the OpenEnroth module found on the
# stack is symbolized. Expect some stale entries from dead frames between the real ones.
#
# Usage: scripts/vita_stack_scan.py <core-dump> <elf> [thread-index] [bytes]

import re
import struct
import subprocess
import sys
from pathlib import Path

VITA_CORE_DUMP = Path("/tmp/vita-core-dump/build/vita-core-dump")
ADDR2LINE = Path.home() / "vitasdk/vitasdk/bin/arm-vita-eabi-addr2line"
READELF = Path.home() / "vitasdk/vitasdk/bin/arm-vita-eabi-readelf"


def run(*args):
    return subprocess.run([str(a) for a in args], capture_output=True, text=True).stdout


def main():
    dump, elf = sys.argv[1], sys.argv[2]
    thread = sys.argv[3] if len(sys.argv) > 3 else "0"
    length = int(sys.argv[4]) if len(sys.argv) > 4 else 16384

    modules = run(VITA_CORE_DUMP, dump, "modules")
    start = int(re.search(r"Module \d+: OpenEnroth\s+Start address: (0x[0-9a-f]+)", modules).group(1), 16)
    text_size = next(int(m.group(2), 16) for m in re.finditer(r"LOAD\s+\S+\s+(0x[0-9a-f]+)\s+\S+\s+\S+\s+(0x[0-9a-f]+)",
                                                             run(READELF, "-lW", elf)))
    delta = start - 0x81000000

    regs = run(VITA_CORE_DUMP, dump, "backtrace", f"--thread={thread}", "--show-registers", f"--add-elf={elf}")
    print(regs.split("\n\n")[0])
    sp = int(re.search(r"sp: (0x[0-9a-f]+)", regs).group(1), 16)

    data = b""
    for line in run(VITA_CORE_DUMP, dump, "memory", f"{sp:x}", f"--length={length}").splitlines():
        m = re.match(r"[0-9a-f]+: ((?:[0-9a-f]{2}\s+)+)\|", line)
        if m:
            data += bytes(int(x, 16) for x in m.group(1).split())

    addresses = []
    for i in range(0, len(data) - 3, 4):
        value = struct.unpack("<I", data[i:i + 4])[0]
        if start <= value < start + text_size and value & 1:
            addresses.append(f"0x{value - delta - 1:x}")

    print(f"\nStack scan of thread {thread}, {len(data)} bytes from sp 0x{sp:x}:")
    if addresses:
        symbols = run(ADDR2LINE, "-f", "-C", "-e", elf, *addresses).splitlines()
        previous = None
        for function, location in zip(symbols[0::2], symbols[1::2]):
            if function != previous and not function.startswith((".L", "??")):
                print(f"  {function}  {location}")
            previous = function


if __name__ == "__main__":
    main()
