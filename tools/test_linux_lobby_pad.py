#!/usr/bin/env python3
"""Exercise the Linux RmlUi controller router under Xvfb without an image or user profile."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, help="configured Linux Ninja build containing libdol.so")
    args = parser.parse_args()
    build = args.build.resolve()
    root = Path(__file__).resolve().parents[1]
    obj = "CMakeFiles/dol.dir/src/port/ui/input.cpp.o"
    command = subprocess.check_output(["ninja", "-C", build, "-t", "commands", "-s", obj], text=True)
    flags = shlex.split(command.splitlines()[-1])
    for option in ("-MT", "-MF", "-o", "-c"):
        index = flags.index(option)
        del flags[index:index + 2]
    flags = [x for x in flags if x not in ("-MD", "-MMD", "-DNDEBUG")]
    with tempfile.TemporaryDirectory(prefix="partyboard-lobby-pad-") as temp:
        exe = Path(temp) / "lobby-pad-test"
        subprocess.run([*flags, "-I", root, root / "tools/tests/linux_lobby_pad_test.cpp",
                        root / "src/port/ui/input.cpp", "-L", build, "-ldol", "-lrmlui", "-lSDL3", "-lX11",
                        "-Wl,-rpath," + str(build), "-o", exe], cwd=build, check=True)
        env = dict(os.environ, XDG_DATA_HOME=temp, XDG_CONFIG_HOME=temp, XDG_CACHE_HOME=temp, SDL_VIDEODRIVER="x11")
        subprocess.run(["xvfb-run", "-a", str(exe), root / "res/Inter-Regular.ttf"], env=env, cwd=temp, check=True)


if __name__ == "__main__":
    main()
