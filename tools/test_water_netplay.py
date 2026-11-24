#!/usr/bin/env python3
"""Run the ROM-free Take a Breather regression against an existing Linux Ninja build.

Usage: python3 tools/test_water_netplay.py build/linux-clang-relwithdebinfo
"""
import argparse
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, help="Linux CMake/Ninja directory containing libdol.so")
    build = parser.parse_args().build.resolve()
    root = Path(__file__).resolve().parents[1]
    stage = root / "src/REL/m456Dll/stage.c"

    # Use the same C compiler, struct layout and regional settings as the game.
    rule = "CMakeFiles/m456Dll.dir/src/REL/m456Dll/stage.c.o"
    commands = subprocess.check_output(["ninja", "-C", build, "-t", "commands", "-s", rule], text=True)
    compiler = shlex.split(commands.strip().splitlines()[-1])
    for flag in ("-MT", "-MF", "-o", "-c"):
        index = compiler.index(flag)
        del compiler[index:index + 2]
    compiler = [arg for arg in compiler if arg not in ("-MD", "-MMD", "-DNDEBUG")]

    with tempfile.TemporaryDirectory(prefix="partyboard-water-") as directory:
        tmp = Path(directory)
        # Elide only the graphics/cache backend. Arguments still evaluate, including
        # gameplay expressions used to produce vertex attributes or GPU commands.
        calls = re.findall(r"\b(GX\w+|HuSprTexLoad|DC(?:FlushRange(?:NoSync)?|InvalidateRange))\(", stage.read_text())
        with (tmp / "water_gpu_stubs.h").open("w") as header:
            header.write("static inline int IgnoreGpuCommand(int unused, ...) { return 64; }\n")
            for call in sorted(set(calls)):
                header.write(f"#undef {call}\n#define {call}(...) IgnoreGpuCommand(0, ##__VA_ARGS__)\n")
        exe = tmp / "water_netplay_test"
        subprocess.run([*compiler, "-ffunction-sections", "-fdata-sections", "-I", str(root), "-I", str(tmp),
                        str(root / "tools/tests/water_netplay_test.c"), "-Wl,--gc-sections", "-Wl,-rpath," + str(build),
                        "-L", str(build), "-ldol", "-lm", "-o", str(exe)], cwd=build, check=True)
        subprocess.run([exe], check=True)


if __name__ == "__main__":
    main()
