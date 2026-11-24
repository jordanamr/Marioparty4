#!/usr/bin/env python3
"""Run the ROM-free board and minigame regressions against an existing Linux Ninja build.

Usage: python3 tools/test_minigame_determinism.py build/linux-clang-relwithdebinfo
"""
import argparse
import json
import time
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, help="Linux CMake/Ninja directory containing libdol.so")
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1),
                        help="concurrent isolated test builds (default up to 4)")
    parser.add_argument("--compiler", choices=["clang", "gcc"], help="override the Linux test compiler")
    parser.add_argument("--ubsan", action="store_true", help="instrument tested C code with UndefinedBehaviorSanitizer")
    parser.add_argument("--report", type=Path, help="write per-test execution results to JSON")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    build = args.build.resolve()
    root = Path(__file__).resolve().parents[1]
    test_cases = [
        ("m403Dll", "main.c", "minigame_camera_path_test.c"),
        ("m406Dll", "player.c", "minigame_avalanche_collision_test.c"),
        ("m407dll", "camera.c", "minigame_domination_camera_test.c"),
        ("m408Dll", "stage.c", "minigame_material_coordinates_test.c"),
        ("m410Dll", "game.c", "three_throw_player_collision_test.c"),
        ("m444dll", "pinball.c", "pinball_velocity_test.c"),
        ("m412Dll", "main.c", "minigame_wall_slide_test.c"),
        ("m421Dll", "player.c", "minigame_direction_grid_test.c"),
        ("w04Dll", "boo_event.c", "boo_train_landing_test.c"),
        ("dol", "game/board/main.c", "board_camera_view_test.c"),
        ("dol", "game/board/com.c", "board_cpu_item_slot_test.c"),
        ("dol", "game/board/space.c", "board_fortune_fixture_test.c"),
        ("dol", "game/board/mg_setup.c", "board_minigame_transition_test.c"),
        ("dol", "game/board/shop.c", "board_shop_input_test.c"),
        ("dol", "game/board/item.c", "board_item_input_test.c"),
        ("dol", "game/board/ui.c", "board_picker_input_test.c"),
        ("dol", "game/board/lottery.c", "board_lottery_input_test.c"),
        ("dol", "game/board/com_path.c", "board_path_search_test.c"),
        ("m462Dll", "main.c", "creature_collision_test.c"),
        ("m456Dll", "stage.c", "water_netplay_test.c"),
        ("m404Dll", "main.c", "trace_race_pixels_test.c"),
        ("m409Dll", "cursor.c", "cursor_lifetime_test.c"),
        ("m453Dll", "score.c", "minigame_score_cleanup_test.c"),
        ("m447dll", "main.c", "minigame_score_sprites_test.c"),
        ("m456Dll", "main.c", "minigame_winner_ties_test.c"),
        ("m415Dll", "map.c", "minigame_stamp_out_layers_test.c"),
        ("m415Dll", "main.c", "stamp_out_ai_test.c"),
        ("m427Dll", "player.c", "course_height_test.c"),
        ("m428Dll", "map.c", "collision_quad_test.c"),
        ("m433Dll", "player.c", "impact_fade_test.c"),
        ("w04Dll", "mg_item.c", "boo_item_cpu_test.c"),
        ("w06Dll", "fire.c", "bowser_inventory_test.c"),
        ("w06Dll", "mg_item.c", "bowser_item_choice_test.c"),
    ]
    def run(case):
        start = time.monotonic()
        module, source, test = case
        result = {"module": module, "source": source, "test": test}
        try:
            compile_and_run(root, build, *case, cc=args.compiler, ubsan=args.ubsan)
        except subprocess.CalledProcessError as error:
            result.update(status="FAIL", exit_code=error.returncode, command=list(map(str, error.cmd)))
        else:
            result.update(status="PASS", exit_code=0)
        return {**result, "seconds": round(time.monotonic() - start, 3)}

    with ThreadPoolExecutor(max_workers=min(args.jobs, len(test_cases))) as pool:
        results = list(pool.map(run, test_cases))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps({"compiler": args.compiler or "configured", "ubsan": args.ubsan,
                                           "tests": results}, indent=2) + "\n")
    failed = [r["test"] for r in results if r["status"] != "PASS"]
    if failed:
        raise SystemExit("FAIL: " + ", ".join(failed))
    print(f"PASS: {len(test_cases)} board and minigame regressions")


def compile_and_run(root, build, module, filename, test, cc=None, ubsan=False):
    source = Path("src") / filename if module == "dol" else Path("src/REL") / module / filename
    stage = root / source
    # Use the same C compiler, struct layout and regional settings as the game.
    rule = f"CMakeFiles/{module}.dir/{source}.o"
    commands = subprocess.check_output(["ninja", "-C", build, "-t", "commands", "-s", rule], text=True)
    compiler = shlex.split(commands.strip().splitlines()[-1])
    for flag in ("-MT", "-MF", "-o", "-c"):
        index = compiler.index(flag)
        del compiler[index:index + 2]
    compiler = [arg for arg in compiler if arg not in ("-MD", "-MMD", "-DNDEBUG")]

    if cc:
        compiler[0] = cc
    if ubsan:
        compiler.extend(["-fsanitize=undefined", "-fno-sanitize-recover=undefined"])

    with tempfile.TemporaryDirectory(prefix="partyboard-minigame-") as directory:
        tmp = Path(directory)
        # Elide only the graphics/cache backend. Arguments still evaluate, including
        # gameplay expressions used to produce vertex attributes or GPU commands.
        calls = re.findall(r"\b(GX\w+|HuSprTexLoad|DC(?:FlushRange(?:NoSync)?|InvalidateRange))\(", stage.read_text())
        with (tmp / "gpu_stubs.h").open("w") as header:
            header.write("static inline int IgnoreGpuCommand(int unused, ...) { return 64; }\n")
            for call in sorted(set(calls)):
                header.write(f"#undef {call}\n#define {call}(...) IgnoreGpuCommand(0, ##__VA_ARGS__)\n")
        exe = tmp / Path(test).stem
        subprocess.run([*compiler, "-ffunction-sections", "-fdata-sections", "-I", str(root), "-I", str(tmp),
                        str(root / "tools/tests" / test), "-Wl,--gc-sections", "-Wl,-rpath," + str(build),
                        "-L", str(build), "-ldol", "-lm", "-o", str(exe)], cwd=build, check=True)
        subprocess.run([exe], check=True)


if __name__ == "__main__":
    main()
