from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
from netplay_free_play import ROUTES, TABS, generate, menu_prefix


class FreePlayTest(unittest.TestCase):
    def test_grid_agrees_with_game_table(self):
        objsub = (TOOLS.parent / "src/game/objsub.c").read_text()
        table = re.findall(r"DLL_m(\d+)dll, // ovl\s*(\d+), // type", objsub)
        self.assertEqual(len(ROUTES), 51)
        self.assertEqual([t for t, _ in TABS], [0, 1, 2, 4, 3, 6, 8])
        for category, modules in TABS:
            self.assertEqual(tuple(int(m) for m, t in table if int(t) == category), modules)

    def test_prefix_reproduces_captured_inputs_of_both_seats(self):
        states = menu_prefix()
        digest = sha256()
        for frame in range(4278):
            for seat in (0, 1):
                buttons, x, y = states.get((frame, seat), (0, 0, 0))
                digest.update(f"{frame} {seat} {buttons} {x} {y} 0 0 0 0\n".encode())
        self.assertEqual(digest.hexdigest(), "52475466692b3faea19371d3baddb8098442a2713a5e14fdbd45138d5b6d22ad")

    def test_menu_pulses_and_confirmation(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "routes"
            files = generate(out, [439], play_frames=8)
            info = json.loads((out / "routes.json").read_text())["cases"][0]
            self.assertEqual((info["game_type"], info["tab"], info["row"]), (4, 3, 2))
            rows = files[439].read_text().splitlines()
            for frame in (4380, 4425, 4470):
                self.assertEqual(rows[2 * frame].split()[2:5], ["0", "100", "0"])
                self.assertEqual(rows[2 * frame + 1].split()[2:5], ["0", "0", "0"])
            for frame in (4515, 4535):
                self.assertEqual(rows[2 * frame].split()[2:5], ["0", "0", "-100"])
            self.assertEqual(rows[info["selection_frame"] * 2].split()[2:5], ["256", "0", "0"])


if __name__ == "__main__":
    unittest.main()
