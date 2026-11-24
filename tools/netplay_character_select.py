#!/usr/bin/env python3
"""Generate PAL/French two-seat Party Mode character-selection replays (delay 3)."""
import argparse
from pathlib import Path

from netplay_free_play import A, B, NEUTRAL, menu_prefix


def write_route(path, actions, automatic_from):
    # Party Mode is already highlighted when the title screen returns control.
    pads = {key: value for key, value in menu_prefix().items() if key[0] < 966}

    def pulse(frame, seat, sample):
        for n in range(frame, frame + 3):
            pads[n, seat] = sample

    for seat in range(2):
        for n in range(963 - 15 * seat, 3087, 120):
            pulse(n, seat, (A, 0, 0))
    for n, seat, button, x, y in actions:
        pulse(n, seat, (button, x, y))
    for n in range(automatic_from, 8500, 50):
        for seat in range(2):
            pulse(n, seat, (A, 0, 0))
    with path.open("w", encoding="ascii") as out:
        for n in range(8500):
            for seat in range(2):
                buttons, x, y = pads.get((n, seat), NEUTRAL)
                out.write(f"{n} {seat} {buttons} {x} {y} 0 0 0 0\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    output = parser.parse_args().output
    output.mkdir(exist_ok=True, parents=True)
    write_route(output / "collision.txt", [
        (3098, 0, 0, 85, 0),     # P1 aims at P2's occupied tile.
        (3125, 0, 0, 0, -85),    # Both cursors change row.
        (3125, 1, 0, 0, -85),
        (3150, 1, A, 0, 0),
        (3160, 1, B, 0, 0),      # Unconfirm before P1 locks.
        (3180, 0, A, 0, 0),
        (3180, 1, A, 0, 0),
    ], 3410)
    backtrack = [(3100, 0, B, 0, 0)]  # Leave the grid for the player-count menu.
    for frame in range(3320, 3621, 50):
        for seat in range(2):
            backtrack.append((frame, seat, A, 0, 0))
    backtrack.extend([
        (3690, 0, 0, 0, -85),
        (3720, 0, A, 0, 0),
        (3740, 0, 0, 85, 0),     # Move the CPU cursor right.
        (3760, 0, B, 0, 0),      # Return from the first CPU to humans.
        (3800, 0, A, 0, 0),
        (3835, 0, A, 0, 0),
        (3870, 0, B, 0, 0),      # Unlock P1 on the revisited human grid.
    ])
    write_route(output / "backtrack.txt", backtrack, 3920)
    write_route(output / "cpu-reselect.txt", [
        (3108, 1, A, 0, 0), (3123, 0, A, 0, 0),
        (3228, 1, A, 0, 0), (3243, 0, A, 0, 0),
        (3270, 0, 0, 0, -85),
        (3300, 0, A, 0, 0),       # Confirm CPU 1, change difficulty, then cancel.
        (3320, 0, 0, 85, 0), (3350, 0, B, 0, 0),
        (3390, 0, A, 0, 0), (3430, 0, A, 0, 0),
        (3480, 0, B, 0, 0),       # Undo CPU 1 from the second CPU's chooser.
    ], 3540)


if __name__ == "__main__":
    main()
