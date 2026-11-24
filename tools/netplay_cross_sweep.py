#!/usr/bin/env python3
"""Run local Linux/Windows PartyBoard sessions and compare both native peers."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from contextlib import ExitStack, contextmanager
from dataclasses import dataclass
import json
import os
from pathlib import Path
import queue
import re
import select
import shutil
import signal
import socket
import subprocess
import sys
import threading
import time

from netplay_sweep_results import classify_case, write_campaign
from netplay_free_play import ROUTES, generate as generate_free_play, module_id


@dataclass(frozen=True)
class Scenario:
    walk: str
    overlay: str
    windows_host: bool
    board: int = None
    context: int = None
    special: int = None


SCENARIOS = {
    "koopa": Scenario("0:2", "w05Dll", True, 4, 93),
    "boo": Scenario("0:1", "w04Dll", False, 3, 92),
    "avalanche": Scenario("2:4", "m406Dll", True),
    "domination": Scenario("2:5", "m407dll", False),
    "stamp-out": Scenario("2:13", "m415Dll", True),
    "take-a-breather": Scenario("2:15", "m456Dll", True),
    "toad": Scenario("0:0", "w01Dll", False, 0, 89),
    "goomba": Scenario("0:-2", "w02Dll", True, 1, 90),
    "shy-guy": Scenario("0:-1", "w03Dll", False, 2, 91),
    "bowser": Scenario("0:3", "w06Dll", True, 5, 94),
}
DEFAULT_SCENARIOS = ("koopa", "boo", "avalanche", "domination", "stamp-out")


def scenario_name(value):
    name = value.lower()
    if name in SCENARIOS:
        return name
    module = re.fullmatch(r"m(4\d\d)(?:dll)?", name)
    if module:
        return "m" + module[1]
    raise argparse.ArgumentTypeError(f"unknown scenario {value!r}; use --list-scenarios or mNNN")


def scenario_definition(name):
    return SCENARIOS.get(name) or Scenario(None, name + "Dll", True)

# Wine prefixes prepared for the distributed MSVC executable contain native
# vkd3d-proton, DXVK and the Visual C++ runtime in system32.
NATIVE_DLLS = ("d3d12,d3d12core,dxgi,d3d11,concrt140,msvcp140,msvcp140_1,"
               "msvcp140_2,msvcp140_atomic_wait,vcruntime140,vcruntime140_1=n")


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--linux-dir", type=Path, help="directory containing the Linux partyboard executable")
    parser.add_argument("--windows-dir", type=Path, help="directory containing partyboard.exe")
    parser.add_argument("--appimage", type=Path, help="Linux distribution as an AppImage")
    parser.add_argument("--windows-zip", type=Path, help="Windows distribution as a ZIP archive")
    parser.add_argument("--package-cache", type=Path, help="reuse extracted archives in this directory (default OUTPUT/packages)")
    parser.add_argument("--rom", type=Path, help="path to the local disc image")
    parser.add_argument("--output", type=Path, help="campaign directory for logs, recordings and results")
    parser.add_argument("--wine-prefix", type=Path, help="copy this configured Wine prefix once per worker")
    parser.add_argument("--jobs", type=int, default=2, help="simultaneous pairs (default 2)")
    parser.add_argument("--scenario", type=scenario_name, action="append", metavar="NAME",
                        help="named scenario or minigame module mNNN with a recording")
    free = parser.add_mutually_exclusive_group()
    free.add_argument("--free-play", type=module_id, action="append", metavar="mNNN",
                      help="generate the PAL/French menu route to this game (repeatable)")
    free.add_argument("--free-play-all", action="store_true", help="run all 51 modules in the seven Free Play tabs")
    free.add_argument("--special", action="append", metavar="mNNN", help="route to one game outside Free Play")
    free.add_argument("--special-all", action="store_true", help="run all ten games outside Free Play")
    parser.add_argument("--board-turn", "--min-turn", type=int, default=2, metavar="N",
                        help="require a matching checkpoint on the selected board at turn N or later (default 2)")
    parser.add_argument("--result-context", type=int, action="append", metavar="ID",
                        help="expected next overlay after a minigame exits (repeatable, default 84)")
    parser.add_argument("--list-scenarios", action="store_true")
    role = parser.add_mutually_exclusive_group()
    role.add_argument("--windows-host", dest="host", action="store_const", const="win")
    role.add_argument("--linux-host", dest="host", action="store_const", const="linux")
    parser.add_argument("--trace-hashes", action="store_true", help="record each native peer’s frame/hash list")
    parser.add_argument("--mixed-graphics", action="store_true",
                        help="keep Linux ultrawide 1x and Windows 4:3 3x even in fast mode")
    parser.add_argument("--fast", action="store_true",
                        help="run unthrottled on smaller displays; requires support in both engines")
    parser.add_argument("--record", action=argparse.BooleanOptionalAction, default=True,
                        help="record both seats from Linux (on by default)")
    parser.add_argument("--replay-input", type=Path, action="append", default=[], metavar="FILE")
    parser.add_argument("--replay-dir", type=Path, help="read a corpus of *.txt inputs for one scenario")
    parser.add_argument("--max-seconds", type=float, default=1350, help="wall-clock limit per case")
    args = parser.parse_args()
    args.compact_video = args.fast and not args.mixed_graphics
    if args.list_scenarios:
        for name, s in SCENARIOS.items():
            print(f"{name:12} {s.walk:5} {s.overlay:9} host={'Windows' if s.windows_host else 'Linux'}")
        print("mNNN         --replay-input FILE or --replay-dir DIR; next overlay defaults to 84")
        parser.exit()
    if not args.rom or not args.output:
        parser.error("--rom and --output are required")
    args.rom = args.rom.resolve()
    args.output = args.output.resolve()
    if not args.rom.is_file():
        parser.error(f"disc image does not exist: {args.rom}")
    if args.board_turn < 1:
        parser.error("--board-turn must be positive")
    if args.jobs < 1 or not (0 < args.max_seconds < float('inf')):
        parser.error("--jobs and --max-seconds must be positive and finite")
    if (args.appimage or args.windows_zip) and (args.linux_dir or args.windows_dir):
        parser.error("choose extracted directories or a pair of archives")
    if not ((args.appimage and args.windows_zip) or (args.linux_dir and args.windows_dir)):
        parser.error("supply --appimage and --windows-zip, or --linux-dir and --windows-dir")
    if args.wine_prefix:
        args.wine_prefix = args.wine_prefix.resolve()
        if (not (args.wine_prefix / "system.reg").is_file() or not (args.wine_prefix / "drive_c").is_dir()
                or (args.wine_prefix / "drive_c").is_symlink()):
            parser.error("--wine-prefix must contain system.reg and drive_c")
        if args.output.is_relative_to(args.wine_prefix):
            parser.error("place --output outside the source Wine prefix")
    if args.free_play_all:
        args.free_play = list(ROUTES)
    args.special_routes = {}
    if args.special or args.special_all:
        from netplay_special_games import SPECIAL_GAMES
        selected = SPECIAL_GAMES.keys() if args.special_all else (
            str(s).lower().removeprefix("m").removesuffix("dll") for s in args.special)
        for value in selected:
            if not str(value).isdecimal() or int(value) not in SPECIAL_GAMES:
                parser.error(f"{value} is not one of the ten special games")
            args.special_routes[f"m{int(value)}"] = SPECIAL_GAMES[int(value)]
    if (args.free_play or args.special_routes) and (args.scenario or args.replay_dir or args.replay_input):
        parser.error("generated routes use their own scenarios and inputs")
    selected = [f"m{n}" for n in args.free_play] if args.free_play else args.special_routes or args.scenario
    args.scenario = list(dict.fromkeys(selected or DEFAULT_SCENARIOS))
    if args.result_context and (min(args.result_context) < 0 or
                               any(scenario_definition(n).board is not None for n in args.scenario)):
        parser.error("--result-context requires a minigame scenario and a non-negative overlay ID")
    args.result_context = tuple(dict.fromkeys(args.result_context or (84,)))
    if args.replay_dir:
        inputs = sorted(args.replay_dir.glob("*.txt"))
        if not inputs:
            parser.error("--replay-dir contains no *.txt recordings")
        args.replay_input.extend(inputs)
    args.replay_input = list(dict.fromkeys(p.resolve() for p in args.replay_input))
    if not (args.free_play or args.special_routes or args.replay_input) and any(name not in SCENARIOS for name in args.scenario):
        parser.error("a module scenario requires --replay-input or --replay-dir")
    if args.replay_input and len(args.scenario) != 1:
        parser.error("supply exactly one --scenario when using recordings")
    for path in args.replay_input:
        if not path.is_file() or not path.stat().st_size:
            parser.error(f"missing or empty replay: {path}")
    return args


def windows_path(path):
    return "Z:" + str(path.resolve()).replace("/", "\\")


def stop_group(process):
    for sig, timeout in ((signal.SIGTERM, 3), (signal.SIGKILL, 3)):
        if process.poll() is not None:
            return
        try:
            os.killpg(process.pid, sig)
        except ProcessLookupError:
            return
        try:
            process.wait(timeout=timeout)
            return
        except subprocess.TimeoutExpired:
            pass


@contextmanager
def display(log, resolution):
    read_fd, write_fd = os.pipe()
    with os.fdopen(read_fd) as reader, ExitStack() as stack:
        try:
            proc = subprocess.Popen(["Xvfb", "-displayfd", str(write_fd), "-screen", "0",
                                     resolution + "x24", "-nolisten", "tcp"],
                                    pass_fds=(write_fd,), stderr=log, start_new_session=True)
        finally:
            os.close(write_fd)
        stack.callback(stop_group, proc)
        if not select.select([reader], [], [], 15)[0]:
            raise RuntimeError("Xvfb did not publish its display number")
        number = reader.readline().strip()
        if not number.isdecimal():
            raise RuntimeError("Xvfb failed to start")
        yield ":" + number


@contextmanager
def worker_environment(directory, source, compact_video):
    directory.mkdir()
    prefix = directory / "wine"
    with ExitStack() as stack:
        log = stack.enter_context((directory / "worker.log").open("wb"))
        stack.callback(shutil.rmtree, prefix, True)
        sizes = {"linux": "640x480" if compact_video else "1260x540", "win": "640x480" if compact_video else "1280x960"}
        displays = {peer: stack.enter_context(display(log, size)) for peer, size in sizes.items()}
        env = dict(os.environ, DISPLAY=displays["win"], WINEPREFIX=str(prefix), WINEDEBUG="-all")
        if source:
            subprocess.run(["cp", "-a", "--reflink=auto", "--", str(source), str(prefix)], check=True,
                           stdout=log, stderr=log)
            # cp preserves absolute symlinks. A C: drive or a registry link
            # into the donor must point into this worker's own clone.
            for parent, directories, files in os.walk(prefix):
                for name in directories + files:
                    link = Path(parent) / name
                    if link.is_symlink():
                        target = link.readlink()
                        if target.is_absolute() and target.is_relative_to(source):
                            link.unlink()
                            link.symlink_to(os.path.relpath(prefix / target.relative_to(source), link.parent))
        else:
            prefix.mkdir()
        # The wineserver is scoped to this fresh, private prefix.
        def close_wine():
            try:
                subprocess.run(["wineserver", "-k"], env=env, stdout=log, stderr=log, timeout=10)
                subprocess.run(["wineserver", "-w"], env=env, stdout=log, stderr=log, timeout=10)
            except subprocess.TimeoutExpired:
                pass
        stack.callback(close_wine)
        if source is None:
            subprocess.run(["wineboot", "--init"], env=env, stdout=log, stderr=log, timeout=120, check=True)
        yield prefix, displays


class PortPool:
    def __init__(self):
        self.lock = threading.Lock()
        self.issued = set()

    def next(self):
        with self.lock:
            while True:
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
                    sock.bind(("127.0.0.1", 0))
                    port = sock.getsockname()[1]
                if port not in self.issued:
                    self.issued.add(port)
                    return port


def launch(peer, host, port, scenario, replay, directory, worker, args, stack, startup_retry=False):
    prefix, displays = worker
    linux = peer == "linux"
    profile = directory / ("linux/MarioPartyRD/Party Board" if linux else "win-profile")
    profile.mkdir(parents=True)
    path = str if linux else windows_path
    config = {"backend.graphicsBackend": "vulkan" if linux else "d3d12", "backend.wasPresetChosen": True,
              "backend.isoPath": path(args.rom), "backend.skipPreLaunchUI": True, "video.enableVsync": not (args.fast or linux),
              "video.enableFullscreen": True, "video.lockAspectRatio": args.compact_video or not linux,
              "video.enableAdaptiveWidescreen": linux and not args.compact_video, "video.keepOriginalHud": linux,
              "game.internalResolutionScale": 1 if (args.compact_video or linux) else 3, "audio.masterVolume": 0}
    (profile / "config.json").write_text(json.dumps(config), encoding="utf-8")
    env = dict(os.environ, DISPLAY=displays[peer], XDG_DATA_HOME=str(directory / peer),
               XDG_CONFIG_HOME=str(directory / peer / "config"), XDG_CACHE_HOME=str(prefix.parent / "cache"),
               SDL_AUDIODRIVER="dummy", PARTYBOARD_NET_DIAGNOSTIC=path(directory / f"{peer}.native"),
               PARTYBOARD_NETPLAY_TEST_PROFILE=path(profile), PARTYBOARD_CRASH_DIR=path(profile))
    if linux:
        if startup_retry:
            # A new display and shader cache isolate a failed graphics startup.
            xvfb_log = stack.enter_context((directory / "linux-xvfb.log").open("wb"))
            env.update(DISPLAY=stack.enter_context(display(xvfb_log, "640x480" if args.compact_video else "1260x540")),
                       XDG_CACHE_HOME=str(directory / "cache"))
        cwd = args.linux_dir
        env.update(SDL_VIDEODRIVER="x11", LD_LIBRARY_PATH=os.pathsep.join(
            str(p) for p in (cwd, cwd.parent / "lib")) + (os.pathsep + os.environ["LD_LIBRARY_PATH"]
                                                       if os.environ.get("LD_LIBRARY_PATH") else ""))
        cmd = [str(cwd / "partyboard")]
    else:
        cwd = args.windows_dir
        env.pop("LD_LIBRARY_PATH", None)
        env.pop("SDL_VIDEODRIVER", None)
        env.update(WINEPREFIX=str(prefix), WINEDEBUG="-all",
                   WINEDLLOVERRIDES="mscoree,mshtml=;" + NATIVE_DLLS if args.wine_prefix else "mscoree,mshtml=")
        cmd = ["wine", str(cwd / "partyboard.exe")]
    cmd += ["--netplay-host", str(port)] if peer == host else ["--netplay-join", f"127.0.0.1:{port}"]
    cmd += ["--netplay-full", "--netplay-loopback", "--netplay-delay", "3", "--netplay-pad",
            "1" if peer == host else "4", "--disc-image", path(args.rom), "--lang=fr"]
    if args.fast:
        cmd += ["--netplay-fast-replay"]
    if scenario.special is not None:
        cmd += ["--netplay-special-game", str(scenario.special)]
    if args.trace_hashes:
        cmd += ["--netplay-record-hashes", path(directory / f"{peer}.hashes")]
    if replay:
        cmd += ["--netplay-replay-input", path(replay)]
    else:
        cmd += ["--netplay-walk-plan=" + scenario.walk]
    if linux and args.record:
        cmd += ["--netplay-record-input", str(directory / "inputs.txt")]
    log = stack.enter_context((directory / f"{peer}.log").open("wb"))
    process = subprocess.Popen(cmd, cwd=cwd, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    stack.callback(stop_group, process)
    return process


def finalize_recording(path):
    """Keep complete frame/seat pairs; an interior defect leaves the original untouched."""
    limits = ((0, 65535), (-128, 127), (-128, 127), (-128, 127), (-128, 127), (0, 255), (0, 255))
    with path.open("r+b") as data:
        size = os.fstat(data.fileno()).st_size
        good_end, frame, seat = 0, 0, 0
        for line in iter(data.readline, b""):
            if not line.endswith(b"\n"):
                # A killed writer may leave even the frame number half written.
                prefix = f"{frame} {seat} ".encode()
                if not prefix.startswith(line):
                    tail = line[len(prefix):].split(b" ")
                    if not line.startswith(prefix) or len(tail) > len(limits):
                        raise ValueError(f"invalid final row at frame {frame}, seat {seat}")
                    for i, (token, (low, high)) in enumerate(zip(tail, limits)):
                        if i == len(tail) - 1 and (token == b"" or (low < 0 and token == b"-")):
                            break
                        if not re.fullmatch(rb"-?\d+", token) or not low <= int(token) <= high:
                            raise ValueError(f"invalid final input at frame {frame}, seat {seat}")
                break
            parts = line.split()
            if len(parts) != 9 or not all(re.fullmatch(rb"-?\d+", token) for token in parts):
                raise ValueError(f"malformed input at frame {frame}, seat {seat}")
            values = list(map(int, parts))
            if values[:2] != [frame, seat]:
                raise ValueError(f"expected frame {frame}, seat {seat}; found {values[:2]}")
            if any(not lo <= value <= hi for value, (lo, hi) in zip(values[2:], limits)):
                raise ValueError(f"input value out of range at frame {frame}, seat {seat}")
            if seat == 1:
                good_end = data.tell()
                frame += 1
            seat ^= 1
        if good_end != size:
            data.truncate(good_end)
        return {"frames": frame, "trimmed_bytes": size - good_end}


class LiveEvidence:
    """Follow appended lines cheaply; the classifier makes the final decision."""
    def __init__(self, directory, peer, scenario, results_contexts=(84,), board_turn=2):
        self.directory = directory
        self.peer = peer
        self.scenario = scenario
        self.results_contexts = results_contexts
        self.board_turn = board_turn
        self.offsets = {}
        self.tails = {}
        self.fast = False
        self.failed = False
        self.first_minigame = None
        self.replay_exhausted = None
        self.special = None
        self.progress = 0
        self.milestone = False

    def poll(self):
        for suffix in ("log", "native"):
            path = self.directory / f"{self.peer}.{suffix}"
            if not path.is_file():
                continue
            with path.open("rb") as source:
                source.seek(self.offsets.get(suffix, 0))
                chunk = source.read()
                self.offsets[suffix] = source.tell()
            if not chunk:
                continue
            lines = (self.tails.get(suffix, b"") + chunk).split(b"\n")
            self.tails[suffix] = lines.pop()
            for line in lines:
                if b"[NET TEST] unthrottled full-game replay" in line:
                    self.fast = True
                if b"[NET TEST] special_game armed module=" in line:
                    match = re.search(rb"special_game armed module=(\d+)", line)
                    if match:
                        self.special = int(match[1])
                if b"DESYNC frame=" in line or b"PROTOCOL frame=" in line:
                    self.failed = True
                if suffix == "log" and self.first_minigame is None and b" prolog end" in line:
                    match = re.search(rb"\bm(4\d\d)dll\.(?:so|dll) prolog end\b", line, re.I)
                    if match:
                        self.first_minigame = int(match[1])
                if suffix == "log" and b"replay_exhausted_frame=" in line:
                    match = re.search(rb"replay_exhausted_frame=(\d+)", line)
                    if match:
                        self.replay_exhausted = int(match[1])
                if suffix == "native" and b"event=checkpoint " in line:
                    fields = dict(re.findall(rb"(\w+)=(-?\d+)\b", line))
                    self.progress = int(fields.get(b"hash_frame", 0))
                    if (self.scenario.board is not None and int(fields.get(b"board", -1)) == self.scenario.board
                            and int(fields.get(b"turn", -1)) >= self.board_turn and int(fields.get(b"context", -1)) == self.scenario.context):
                        self.milestone = True
                elif suffix == "log" and self.scenario.board is None and b"Start New OVL " in line:
                    transition = re.search(rb"Start New OVL (\d+)\b", line)
                    if transition and int(transition[1]) in self.results_contexts:
                        self.milestone = True


def free_play_failure(module, monitors):
    loaded = {peer: monitor.first_minigame for peer, monitor in monitors.items()}
    if all(n is not None for n in loaded.values()) and any(n != module for n in loaded.values()):
        return {"reason": "wrong_minigame", "observed_modules": loaded}
    exhausted = {peer: monitor.replay_exhausted for peer, monitor in monitors.items()}
    if module not in loaded.values() and all(f is not None for f in exhausted.values()):
        return {"reason": "replay_exhausted_before_minigame", "replay_exhausted_frames": exhausted}
    return None


def linux_startup_stalled(directory):
    linux_log = directory / "linux.log"
    win_native = directory / "win.native"
    return (linux_log.is_file() and linux_log.stat().st_size == 0 and not (directory / "linux.native").exists()
            and win_native.is_file() and win_native.stat().st_size > 0)


def run_case(job, worker, args, ports, stop, deadline, startup_retry=False):
    case_id, name, replay = job
    scenario = scenario_definition(name)
    results_contexts = args.result_context
    if name in args.special_routes:
        route = args.special_routes[name]
        scenario = Scenario(route["walk"], scenario.overlay, True, special=int(name[1:]))
        results_contexts = route["results"]
    directory = args.output / "cases" / case_id
    if startup_retry:
        directory = directory / "startup-retry"
    directory.mkdir()
    host = args.host or ("win" if scenario.windows_host else "linux")
    peer = "linux" if host == "win" else "win"
    port = ports.next()
    options = dict(overlay=scenario.overlay, board_id=scenario.board, context=scenario.context, min_turn=args.board_turn,
                   results_contexts=results_contexts, direct_return=scenario.special is not None)
    print(f"START {case_id} host={host}", flush=True)
    started = time.monotonic()
    monitors = {side: LiveEvidence(directory, side, scenario, results_contexts, args.board_turn) for side in (host, peer)}
    with ExitStack() as processes:
        players = {host: launch(host, host, port, scenario, replay, directory, worker, args, processes, startup_retry)}
        stop.wait(0.3)
        players[peer] = launch(peer, host, port, scenario, replay, directory, worker, args, processes, startup_retry)
        while True:
            codes = {side: p.poll() for side, p in players.items()}
            expired = time.monotonic() >= deadline
            for monitor in monitors.values():
                monitor.poll()
            fast_seen = {side for side, m in monitors.items() if m.fast}
            route_failure = free_play_failure(int(name[1:]), monitors) if args.free_play else None
            if (time.monotonic() - started >= 45 and not stop.is_set()
                    and all(code is None for code in codes.values()) and linux_startup_stalled(directory)):
                route_failure = {"reason": "linux_startup_stalled"}
            unconfirmed = args.fast and len(fast_seen) != 2 and all(m.progress >= 240 for m in monitors.values())
            if (scenario.special is not None and all(m.progress >= 240 for m in monitors.values())
                    and any(m.special != scenario.special for m in monitors.values())):
                route_failure = {"reason": "special_route_unconfirmed"}
            final = expired or stop.is_set() or any(code is not None for code in codes.values())
            if final or unconfirmed or route_failure or any(m.failed for m in monitors.values()) or all(m.milestone for m in monitors.values()):
                result = classify_case(directory, **options, timed_out=expired, exit_codes=codes)
                if result["status"] == "DESYNC":
                    # The other peer may still be writing its independent field report.
                    until = time.monotonic() + 3
                    while time.monotonic() < until and len(list(directory.rglob("netplay_desync_*.log"))) < 2:
                        if stop.wait(0.1):
                            break
                    result = classify_case(directory, **options, timed_out=expired, exit_codes=codes)
                    break
                if route_failure and result["status"] not in {"CRASH", "DESYNC"} and not result["first_divergence"]:
                    result.update(status="INCOMPLETE", **route_failure)
                    break
                if unconfirmed and result["status"] != "CRASH" and not result["first_divergence"]:
                    result.update(status="INCOMPLETE", reason="fast_mode_unconfirmed")
                    break
                if result["status"] != "INCOMPLETE" or result["reason"] == "PROTOCOL" or final:
                    break
            stop.wait(min(1, max(0, deadline - time.monotonic())))
    elapsed = max(0.001, time.monotonic() - started)
    frames = result["last_matching_checkpoint"]
    result.update(id=case_id, scenario=name, host=host, elapsed_seconds=round(elapsed, 3),
                  matched_frames_per_second=round((frames + 1) / elapsed, 2) if frames is not None else None,
                  fast_requested=args.fast, fast_confirmed=sorted(fast_seen), replay=str(replay) if replay else None,
                  recording=str(directory / "inputs.txt") if (directory / "inputs.txt").is_file() else None)
    issues = []
    if args.record:
        try:
            result["recording_integrity"] = finalize_recording(directory / "inputs.txt")
            if result["recording_integrity"]["frames"] == 0:
                issues.append("empty_recording")
        except (OSError, ValueError) as error:
            result["recording_error"] = str(error)
            issues.append("invalid_recording")
    if args.trace_hashes:
        result["hash_traces"] = {side: f"{side}.hashes" for side in (host, peer)
                                 if (directory / f"{side}.hashes").is_file() and (directory / f"{side}.hashes").stat().st_size > 0}
        if len(result["hash_traces"]) != 2:
            issues.append("missing_hash_trace")
        elif result["status"] == "PASS":
            verify_hash_traces(directory, result, issues)
    if issues:
        result["evidence_issues"] = issues
        if result["status"] == "PASS":
            result.update(status="INCOMPLETE", reason=issues[0])
    (directory / "case.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def verify_hash_traces(directory, result, issues):
    frame = result["verified_frame"]
    # The comparison CLI writes its diagnostic to stdout/stderr. Keep each
    # worker's evidence separate, including when multiple cases finish together.
    compare = Path(__file__).with_name("netplay_compare.py")
    check = subprocess.run([sys.executable, str(compare), "--hash-traces", "--through", str(frame),
                            str(directory / "linux.hashes"), str(directory / "win.hashes")],
                           capture_output=True, text=True)
    message = (check.stdout + check.stderr).strip()
    result["hash_comparison"] = {"through": frame, "message": message}
    if check.returncode == 1:
        match = re.search(r"DESYNC at frame (\d+)", message)
        result.update(status="DESYNC", reason="hash_trace_mismatch", first_divergence={
            "frame": int(match[1]) if match else None, "source": "per_frame_hashes"})
    elif check.returncode != 0:
        issues.append("incomplete_hash_trace")


def main():
    args = arguments()
    for command in ("wine", "wineserver", "Xvfb", "cp" if args.wine_prefix else "wineboot"):
        if not shutil.which(command):
            raise ValueError(f"required command not found: {command}")
    args.output.mkdir(parents=True, exist_ok=True)
    if args.appimage:
        from netplay_sweep_packages import prepare_packages
        cache = (args.package_cache or args.output / "packages").resolve()
        args.linux_dir, args.windows_dir = prepare_packages(args.appimage, args.windows_zip, cache)
    args.linux_dir, args.windows_dir = args.linux_dir.resolve(), args.windows_dir.resolve()
    for p in (args.linux_dir / "partyboard", args.windows_dir / "partyboard.exe"):
        if not p.is_file():
            raise ValueError(f"missing game executable: {p}")
    for name in args.scenario:
        if name not in SCENARIOS:
            module = scenario_definition(name).overlay.lower()
            for extension, dirs in ((".so", [args.linux_dir, args.linux_dir.parent / "lib"]),
                                    (".dll", [args.windows_dir])):
                if not any(p.name.lower() == module + extension for d in dirs if d.is_dir() for p in d.iterdir()):
                    raise ValueError(f"module {module + extension} is missing from the supplied game")
    # mkdir is atomic across processes and keeps previous campaign evidence intact.
    (args.output / "cases").mkdir()
    work = args.output / "workers"
    work.mkdir()
    jobs = queue.Queue()
    free_inputs = generate_free_play(args.output / "free-play", dict.fromkeys(args.free_play)) if args.free_play else {}
    for name in args.scenario:
        if free_inputs:
            jobs.put((name, name, free_inputs[int(name[1:])]))
            continue
        for n, replay in enumerate(args.replay_input or [None], 1):
            suffix = "" if replay is None else f"-{n:03d}-" + re.sub(r"[^a-zA-Z0-9_-]", "_", replay.stem)
            jobs.put((name + suffix, name, replay))
    results, lock, ports, stop = [], threading.Lock(), PortPool(), threading.Event()

    def worker(index):
        directory = work / f"worker-{index}"
        with worker_environment(directory, args.wine_prefix, args.compact_video) as env:
            while not stop.is_set():
                try:
                    job = jobs.get_nowait()
                except queue.Empty:
                    return
                try:
                    started = time.monotonic()
                    deadline = started + args.max_seconds
                    result = run_case(job, env, args, ports, stop, deadline)
                    if (result.get("reason") == "linux_startup_stalled" and not stop.is_set()
                            and time.monotonic() < deadline):
                        first = result
                        print(f"RETRY {job[0]} (Linux startup stalled)", flush=True)
                        result = run_case(job, env, args, ports, stop, deadline, startup_retry=True)
                        result["first_attempt"] = str(Path(first["run_dir"]) / "case.json")
                        result["total_elapsed_seconds"] = round(time.monotonic() - started, 3)
                        case_file = Path(result["run_dir"]) / "case.json"
                        case_file.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
                except (OSError, RuntimeError, subprocess.SubprocessError) as e:
                    result = {"id": job[0], "status": "CRASH", "error": str(e), "scenario": job[1],
                              "directory": str(args.output / "cases" / job[0])}
                with lock:
                    results.append(result)
                    write_campaign(args.output, results)
                    print(f"{result['status']} {job[0]}", flush=True)

    def terminate(_signum, _frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, terminate)
    count = min(args.jobs, jobs.qsize())
    with ThreadPoolExecutor(max_workers=count) as pool:
        futures = [pool.submit(worker, i + 1) for i in range(count)]
        try:
            for future in futures:
                future.result()
        finally:
            stop.set()
    return 0 if all(case["status"] == "PASS" for case in results) else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        sys.exit(f"netplay_cross_sweep: {error}")
