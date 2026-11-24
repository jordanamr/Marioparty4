import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from generate_input import main, PULSES


class VariantTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.source = self.root / "recording.txt"
        self.data = b"".join(
            f"{frame}\t{seat}  256 -72 72 34 -12 17 55\r\n".encode()
            for frame in range(20) for seat in (0, 1)
        )
        self.source.write_bytes(self.data)

    def generate(self, directory, *options):
        dest = self.root / directory
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            main(["variants", str(self.source), "-o", str(dest), *options])
        return json.loads((dest / "manifest.json").read_bytes())

    def test_seeds_manifest_and_windows(self):
        options = ["--window", "3:8", "--window", "14:18", "--seat", "1", "--count", "24"]
        first = self.generate("a", *options, "--seed", "37")
        repeat = self.generate("b", *options, "--seed", "37")
        other = self.generate("c", *options, "--seed", "38")
        self.assertEqual(first, repeat)
        self.assertNotEqual(first["variants"], other["variants"])
        self.assertEqual(len({v["sha256"] for v in first["variants"]}), 24)
        self.assertEqual(first["source_sha256"], hashlib.sha256(self.data).hexdigest())
        for case in first["variants"]:
            output = (self.root / "a" / case["file"]).read_bytes()
            self.assertEqual(output, (self.root / "b" / case["file"]).read_bytes())
            self.assertEqual(case["sha256"], hashlib.sha256(output).hexdigest())
            self.assertEqual(len(case["pulses"]), 2)
            self.assertEqual(len(output.splitlines()), len(self.data.splitlines()))
            for (lo, hi), pulse in zip(first["windows"], case["pulses"]):
                self.assertLessEqual(lo, pulse["start"])
                self.assertLess(pulse["start"], pulse["end"])
                self.assertLessEqual(pulse["end"], hi)
            for old, new in zip(self.data.splitlines(True), output.splitlines(True)):
                frame, seat, *_ = map(int, old.split())
                changed = seat == 1 and any(p["start"] <= frame < p["end"] for p in case["pulses"])
                if changed:
                    self.assertEqual(sum(a != b for a, b in zip(old.split(), new.split())), 1)
                    self.assertEqual(old.split()[-4:], new.split()[-4:])
                    self.assertTrue(new.endswith(b"\r\n"))
                else:
                    self.assertEqual(old, new)
        self.assertEqual(self.source.read_bytes(), self.data)

    def test_sparse_recording_and_last_duplicate(self):
        duplicate = b"8 0 7 6 5 4 3 2 1\n"
        last = b"8 0 0 0 0 0 0 0 0\n"
        other = b"9 1 0 0 0 0 0 0 0\n"
        truncated = b"99 0 256"
        self.source.write_bytes(duplicate + other + last + truncated)
        manifest = self.generate("all", "--window", "8:10", "--seat", "0", "--count", str(len(PULSES)))
        self.assertEqual(len({v["sha256"] for v in manifest["variants"]}), len(PULSES))
        for case in manifest["variants"]:
            payload = (self.root / "all" / case["file"]).read_bytes()
            self.assertTrue(payload.startswith(duplicate + other))
            self.assertTrue(payload.endswith(truncated))
            self.assertEqual(len(payload.splitlines()), 4)
            self.assertEqual((case["pulses"][0]["start"], case["pulses"][0]["end"]), (8, 10))

    def test_both_seats_are_mutated(self):
        case = self.generate("both", "--window", "1:2", "--count", "1")["variants"][0]
        lines = (self.root / "both" / case["file"]).read_bytes().splitlines(True)
        original = self.data.splitlines(True)
        self.assertEqual([i for i in range(len(lines)) if original[i] != lines[i]], [2, 3])

    def test_rejects_invalid_ranges_and_mixed_corpus(self):
        for args in (["--window", "8:2"], ["--window", "8:21"], ["--window", "1:5", "--window", "4:7"],
                     ["--window", "3:4", "--count", "12"], ["--window", "0:2", "--hold", "0"]):
            with self.subTest(args=args), self.assertRaises(SystemExit):
                self.generate("invalid", *args)
            self.assertFalse((self.root / "invalid").exists())
        self.source.write_bytes(b"5 1 0 0 0 0 0 0 0\n")
        with self.assertRaises(SystemExit):
            self.generate("invalid", "--window", "5:6", "--seat", "0")
        self.generate("existing", "--window", "5:6")
        manifest = (self.root / "existing" / "manifest.json").read_bytes()
        with self.assertRaises(SystemExit):
            self.generate("existing", "--window", "5:6", "--seed", "3")
        self.assertEqual(manifest, (self.root / "existing" / "manifest.json").read_bytes())


if __name__ == "__main__":
    unittest.main()
