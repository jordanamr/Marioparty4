"""Recordings and live evidence used by the local two-engine campaign driver."""
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from netplay_cross_sweep import LiveEvidence, SCENARIOS, finalize_recording, free_play_failure, scenario_definition, scenario_name, verify_hash_traces


class CampaignEvidenceTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.directory = Path(temp.name)
        self.path = self.directory / "inputs.txt"
        self.pair = b"0 0 256 -128 127 0 0 255 0\n0 1 0 0 0 0 0 0 0\n"

    def test_partial_last_pair_is_removed(self):
        first = b"1 0 0 0 0 0 0 0 0\n"
        for tail in (b"", b"1", first, first + b"1 1 0 -"):
            with self.subTest(tail=tail):
                self.path.write_bytes(self.pair + tail)
                self.assertEqual(finalize_recording(self.path), {"frames": 1, "trimmed_bytes": len(tail)})
                self.assertEqual(self.path.read_bytes(), self.pair)

    def test_corruption_preserves_the_evidence(self):
        for tail in (b"2 0 0 0 0 0 0 0 0\n", b"1 1 0 0 0 0 0 0 0\n", b"1 0 99999 0 0 0 0 0 0\n",
                     b"1 0 8\n", b"1 0 0 400"):
            with self.subTest(tail=tail):
                content = self.pair + tail
                self.path.write_bytes(content)
                with self.assertRaises(ValueError):
                    finalize_recording(self.path)
                self.assertEqual(self.path.read_bytes(), content)

    def test_append_only_logs_detect_split_markers(self):
        monitor = LiveEvidence(self.directory, "linux", SCENARIOS["koopa"])
        log = self.directory / "linux.log"
        log.write_bytes(b"[NET TEST] unthrottled full-")
        monitor.poll()
        self.assertFalse(monitor.fast)
        with log.open("ab") as stream:
            stream.write(b"game replay\n")
        (self.directory / "linux.native").write_bytes(
            b"event=checkpoint hash_frame=960 context=93 turn=1 board=4\n"
            b"event=checkpoint hash_frame=1080 context=92 turn=2 board=3\n")
        monitor.poll()
        self.assertTrue(monitor.fast)
        self.assertFalse(monitor.milestone)
        with (self.directory / "linux.native").open("ab") as stream:
            stream.write(b"event=checkpoint hash_frame=1200 context=93 turn=2 board=4\n")
        monitor.poll()
        self.assertEqual(monitor.progress, 1200)
        self.assertTrue(monitor.milestone)

    def test_custom_minigame_waits_for_exact_result_overlay(self):
        scenario = scenario_definition(scenario_name("m403DLL"))
        self.assertEqual(scenario.overlay, "m403Dll")
        self.assertIsNone(scenario.walk)
        monitor = LiveEvidence(self.directory, "linux", scenario, (90,))
        log = self.directory / "linux.log"
        log.write_bytes(b"Start New OVL 84\nStart New OVL 900\n")
        monitor.poll()
        self.assertFalse(monitor.milestone)
        with log.open("ab") as stream:
            stream.write(b"Start New OVL 90\n")
        monitor.poll()
        self.assertTrue(monitor.milestone)

    def test_generated_route_waits_for_both_peers_before_reporting_wrong_game(self):
        scenario = scenario_definition("m403")
        monitors = {p: LiveEvidence(self.directory, p, scenario) for p in ("linux", "win")}
        (self.directory / "linux.log").write_bytes(b"objdll> m407dll.so prolog end\n")
        monitors["linux"].poll()
        self.assertIsNone(free_play_failure(403, monitors))
        (self.directory / "win.log").write_bytes(b"objdll> m407dll.dll prolog end\n")
        monitors["win"].poll()
        self.assertEqual(free_play_failure(403, monitors),
                         {"reason": "wrong_minigame", "observed_modules": {"linux": 407, "win": 407}})
        self.assertIsNone(free_play_failure(407, monitors))

    def test_generated_route_can_exhaust_before_entering_game(self):
        scenario = scenario_definition("m403")
        monitors = {p: LiveEvidence(self.directory, p, scenario) for p in ("linux", "win")}
        for p, monitor in monitors.items():
            (self.directory / (p + ".log")).write_bytes(b"[NET TEST] replay_exhausted_frame=6000\n")
            monitor.poll()
        self.assertEqual(free_play_failure(403, monitors)["reason"], "replay_exhausted_before_minigame")

    def test_pass_requires_both_contiguous_hash_traces(self):
        rows = b"0 000000000000000a\n1 000000000000000b\n"
        (self.directory / "linux.hashes").write_bytes(rows)
        for right, expected, issues in ((rows, "PASS", []), (rows[:19], "PASS", ["incomplete_hash_trace"]),
                                         (rows.replace(b"000b", b"000c"), "DESYNC", [])):
            with self.subTest(right=right):
                (self.directory / "win.hashes").write_bytes(right)
                result = {"status": "PASS", "verified_frame": 1}
                actual_issues = []
                verify_hash_traces(self.directory, result, actual_issues)
                self.assertEqual((result["status"], actual_issues), (expected, issues))
                if expected == "DESYNC":
                    self.assertEqual(result["first_divergence"]["frame"], 1)


if __name__ == "__main__":
    unittest.main()
