import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from netplay_sweep_results import classify_case, write_campaign


ENTER = "Start New OVL 15\nobjdll> m407dll.so prolog end\n"
EXIT = "odjdll>Unlink DLL:m407dll.so\n"
RESULT = "Start New OVL 84\nStart New OVL 72\nNetplay: game context 72 at network frame 210.\n"


def checkpoint(frame, context, h="a" * 16, extra=""):
    return f"event=checkpoint hash_frame={frame} state_hash={h} hash_version=4 context={context} {extra}\n"


class ResultsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def logs(self, log, native, peers=("linux", "win")):
        for peer in peers:
            (self.root / f"{peer}.log").write_text(log)
            (self.root / f"{peer}.native").write_text(native)

    def case(self, overlay=407, **kwargs):
        return classify_case(self.root, overlay=overlay, **kwargs)

    def test_success_requires_play_results_and_later_equal_checkpoint(self):
        start = checkpoint(0, 72) + checkpoint(120, 15)
        self.logs(ENTER + EXIT + RESULT, start)
        self.assertEqual(self.case()["status"], "INCOMPLETE")
        self.logs(ENTER + EXIT + RESULT, start + checkpoint(240, 72))
        result = self.case()
        self.assertEqual((result["status"], result["verified_frame"]), ("PASS", 240))
        (self.root / "win.log").write_text(ENTER + EXIT + "Start New OVL 72\n" + RESULT)
        self.assertEqual(self.case()["status"], "INCOMPLETE")

    def test_staying_in_results_does_not_count_as_a_return(self):
        log = ENTER + EXIT + "Start New OVL 84\nNetplay: game context 84 at network frame 180.\n"
        self.logs(log, checkpoint(120, 15) + checkpoint(240, 84))
        self.assertEqual(self.case()["status"], "INCOMPLETE")

    def test_direct_return_requires_exit_and_a_new_checkpoint_in_destination(self):
        params = {"results_contexts": (75,), "direct_return": True}
        target = "Start New OVL 75\nNetplay: game context 75 at network frame 180.\n"
        within_game = checkpoint(0, 75) + checkpoint(120, 15)
        self.logs(ENTER + EXIT + target, within_game)
        self.assertEqual(self.case(**params)["status"], "INCOMPLETE")
        self.logs(ENTER + EXIT + target, within_game + checkpoint(240, 75))
        self.assertEqual(self.case(**params)["reason"], "minigame_direct_return")
        self.assertEqual(self.case(results_contexts=(75,))["status"], "INCOMPLETE")
        (self.root / "win.log").write_text(ENTER + target)
        self.assertEqual(self.case(**params)["status"], "INCOMPLETE")

    def test_crash_timeout_or_overlay_only(self):
        self.logs(ENTER, checkpoint(120, 15))
        self.assertEqual(self.case(timed_out=True)["status"], "TIMEOUT")
        self.assertEqual(self.case()["reason"], "overlay_only")
        self.assertEqual(self.case(exit_codes={"win": -11})["status"], "CRASH")

    def test_first_desync_has_verified_field_and_digests(self):
        self.logs(ENTER, checkpoint(0, 15))
        for peer, player, local, remote in (("linux", 0, "a", "b"), ("win", 1, "b", "a")):
            folder = self.root / peer
            folder.mkdir()
            (self.root / f"{peer}.native.desync").write_text(
                f"DESYNC frame=41 category=ANIMATION localHash={local * 16} remoteHash={remote * 16}\n"
                "DESYNC frame=43 category=SCENE\n")
            (folder / "netplay_desync_first.log").write_text(
                f"session=abc player={player}\nfirst_desync_frame=41 last_good_frame=40\n"
                f"FIELDS frame=41 count=1\nFIELD 0 ANIMATION pos.x {local * 8}\n")
        result = self.case(timed_out=True)
        self.assertEqual(result["status"], "DESYNC")
        self.assertEqual(result["last_equal_frame"], 40)
        first = result["first_divergence"]
        self.assertEqual((first["frame"], first["category"], first["field"]["name"]), (41, "ANIMATION", "pos.x"))
        self.assertEqual(first["hashes"], {"linux": "a" * 16, "win": "b" * 16})
        (self.root / "win/netplay_desync_first.log").unlink()
        self.assertIsNone(self.case()["first_divergence"]["field"])

    def test_checkpoint_conflict_without_reports(self):
        self.logs(ENTER, checkpoint(0, 15) + checkpoint(120, 15))
        (self.root / "win.native").write_text(checkpoint(0, 15) + checkpoint(120, 15, "c" * 16))
        result = self.case()
        self.assertEqual(result["status"], "DESYNC")
        self.assertEqual(result["first_divergence"]["frame"], 120)
        self.assertIsNone(result["first_divergence"]["field"])
        self.assertEqual(result["last_equal_frame"], 0)

    def test_partial_board_progress_counts_only_equal_turns_on_both_peers(self):
        board = "Start New OVL 93\nobjdll> w05Dll.so prolog end\n"
        common = checkpoint(120, 93, extra="board=4 turn=1") + checkpoint(240, 93, extra="board=4 turn=2")
        self.logs(board, common + checkpoint(360, 93, extra="board=4 turn=4"))
        (self.root / "win.native").write_text(common + checkpoint(360, 93, extra="board=4 turn=3"))
        result = self.case("w05Dll", board_id=4, min_turn=4, context=93, timed_out=True)
        self.assertEqual(result["status"], "TIMEOUT")
        self.assertEqual(result["highest_verified_turn"], 2)
        self.assertEqual(result["board_turn_checkpoints"], {1: 120, 2: 240})

    def test_board_checkpoint_requires_target_turn_board_and_both_peers(self):
        board = "Start New OVL 93\nobjdll> w05Dll.so prolog end\n"
        self.logs(board, checkpoint(120, 93, extra="board=4 turn=1"))
        params = {"board_id": 4, "min_turn": 2, "context": 93}
        self.assertEqual(self.case("w05Dll", **params)["status"], "INCOMPLETE")
        self.logs(board, checkpoint(240, 93, extra="board=4 turn=2"))
        self.assertEqual(self.case("w05Dll", **params)["reason"], "board_turn")
        (self.root / "win.native").write_text(checkpoint(240, 93, extra="board=3 turn=2"))
        self.assertEqual(self.case("w05Dll", **params)["status"], "INCOMPLETE")

    def test_manifest_keeps_individual_runs_and_status_counts(self):
        rows = [dict(self.case(), id="B"), dict(self.case(), id="A")]
        output = write_campaign(self.root, rows)
        data = json.loads(output.read_text())
        self.assertEqual([r["id"] for r in data["cases"]], ["A", "B"])
        self.assertEqual(data["counts"], {"INCOMPLETE": 2})


if __name__ == "__main__":
    unittest.main()
