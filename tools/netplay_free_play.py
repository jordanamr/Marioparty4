#!/usr/bin/env python3
"""Generate two-seat PAL/French Free Play input recordings from the title screen."""
import argparse
import json
from pathlib import Path

# fn_1_B770 in mgmodedll/free_play.c sets tab order. fn_1_279FC fills each
# list in mgInfoTbl order (game/objsub.c), including coin minigames here.
TABS = (
    (0, (401, 402, 403, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 443, 456)),
    (1, (416, 417, 418, 419, 420, 421, 422, 423, 424)),
    (2, (425, 426, 427, 428, 429, 430, 431, 432, 434)),
    (4, (404, 438, 439, 440, 441, 455)),
    (3, (435, 436, 437)),
    (6, (445, 446, 447, 448, 449)),
    (8, (450, 457, 458)),
)
ROUTES = {module: (tab, row) for tab, (_, modules) in enumerate(TABS) for row, module in enumerate(modules)}
A, B, START = 256, 512, 4096
NEUTRAL = (0, 0, 0)


def module_id(value):
    name = value.lower().removeprefix("m").removesuffix("dll")
    if name.isdecimal() and int(name) in ROUTES:
        return int(name)
    raise argparse.ArgumentTypeError(f"{value} is not in the seven Free Play tabs")


def menu_prefix():
    """Non-neutral, three-frame pulses captured from a PAL/fr 2:0 walk, delay 3."""
    pads = {}
    def pulse(seat, times, state, duration=3):
        for start in times:
            for frame in range(start, start + duration):
                pads[frame, seat] = state
    pulse(0, (3, 33, 93, 213, 243, 273, 333, 453, 483, 513, 573, 693, 723, 753, 813, 933,
              *range(963, 1800, 120), *range(1923, 4278, 120)), (A, 0, 0))
    pulse(0, [1804], (A, 0, 0), 2)
    pulse(0, [1803], (0, 0, -100), 1)
    pulse(0, range(63, 900, 240), (START, 0, 0))
    pulse(0, range(123, 900, 240), (0, 0, -100))
    pulse(0, (183, 423, 663, 903, 1220, 1283), (0, 100, 0))
    pulse(0, (393, 873), (B, 0, 0))
    pulse(1, (18, 78, 198, 228, 258, 318, 438, 468, 498, 558, 678, 708, 738, 798, 918,
              948, 1068, 1188, *range(1908, 4278, 120)), (A, 0, 0))
    pulse(1, range(48, 900, 240), (START, 0, 0))
    pulse(1, range(108, 900, 240), (0, 0, -100))
    pulse(1, range(168, 900, 240), (0, 100, 0))
    pulse(1, (378, 858), (B, 0, 0))
    return pads


def write_route(path, module, play_frames):
    tab, row = ROUTES[module]
    pads = menu_prefix()
    # The PAL/fr list becomes interactive at frame 4278. Seat zero owns the
    # cursor; changing category sets its row to zero. One analog pulse per move.
    frame = 4380
    for moves, state, spacing in ((tab, (0, 100, 0), 45), (row, (0, 0, -100), 20)):
        for _ in range(moves):
            for i in range(3):
                pads[frame + i, 0] = state
            frame += spacing
    selected = frame
    with path.open("w", encoding="ascii", newline="\n") as stream:
        for frame in range(selected + play_frames):
            for seat in (0, 1):
                if frame < selected:
                    buttons, x, y = pads.get((frame, seat), NEUTRAL)
                else:
                    # Confirm team assignment and dialogs with A. The instructions
                    # require START alone. Native replay masks START during gameplay.
                    elapsed = frame - selected
                    beat = elapsed + 7 * seat
                    buttons = A if beat % 16 < 2 else 0
                    x = y = 0
                    if elapsed >= 300 and beat % 120 in (62, 63):
                        buttons = START
                    elif elapsed >= 1200:
                        # Exercise both sticks' primary directions and a second
                        # action during play without backing out of team selection.
                        x, y = ((85, 0), (0, 85), (-85, 0), (0, -85))[(beat // 37) % 4]
                        if beat % 80 in (25, 26):
                            buttons = B
                stream.write(f"{frame} {seat} {buttons} {x} {y} 0 0 0 0\n")
    return {"module": module, "game_type": TABS[tab][0], "tab": tab, "row": row,
            "selection_frame": selected, "frames": selected + play_frames, "file": path.name}


def generate(directory, modules, play_frames=24000):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=False)
    entries = []
    for module in modules:
        entries.append(write_route(directory / f"m{module}.txt", module, play_frames))
    manifest = {"region": "PAL", "language": "fr", "delay": 3, "cases": entries}
    (directory / "routes.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return {entry["module"]: directory / entry["file"] for entry in entries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True, help="new directory for native replay files and routes.json")
    parser.add_argument("--module", type=module_id, action="append", help="one Free Play module (repeatable, default all 51)")
    parser.add_argument("--play-frames", type=int, default=24000)
    args = parser.parse_args()
    if args.play_frames < 1:
        parser.error("--play-frames must be positive")
    generate(args.output, dict.fromkeys(args.module or ROUTES), args.play_frames)


if __name__ == "__main__":
    main()
