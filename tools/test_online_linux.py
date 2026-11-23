#!/usr/bin/env python3
"""Start a private desktop lobby, launch real games, and check that they exit.

Run on a Linux desktop with a private IPv4 route and a graphics backend:
  python3 tools/test_online_linux.py path/to/PartyBoardOnline path/to/game.rvz --players 4
Use xvfb-run for an offscreen Linux build with software rendering.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time


class Seat:
    def __init__(self, index, root, companion, disc, count, invitation):
        self.room = root / str(index)
        self.room.mkdir(mode=0o700)
        data = self.room / "data"
        data.mkdir()
        environment = {
            **os.environ,
            "PARTYBOARD_ONLINE_DISC": str(disc),
            "PARTYBOARD_ONLINE_NICKNAME": f"Player{index + 1}",
            "PARTYBOARD_ONLINE_PLAYERS": str(count),
            "PARTYBOARD_ONLINE_STATE_DIR": str(self.room),
            "PARTYBOARD_ONLINE_PARENT_PID": str(os.getpid()),
            "PARTYBOARD_ONLINE_INVITE": invitation or "",
            "PARTYBOARD_RELAY": "",
            "PARTYBOARD_ONLINE_NO_SYSTEM_CHANGES": "1",
            "XDG_DATA_HOME": str(data),
            "XDG_CONFIG_HOME": str(self.room / "config"),
            "XDG_CACHE_HOME": str(self.room / "cache"),
        }
        with (self.room / "process.log").open("wb") as log:
            self.process = subprocess.Popen(
                [companion, "--join" if invitation else "--host"], env=environment,
                stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
            )

    def state(self):
        try:
            state = json.loads((self.room / "room.json").read_text())
        except FileNotFoundError:
            state = {}
        if state.get("state") == "error":
            raise RuntimeError(state.get("message"))
        if self.process.poll() is not None and state.get("state") != "closed":
            raise RuntimeError(f"Companion exited with status {self.process.returncode}")
        return state

    def request(self, action):
        folder = self.room / "requests"
        folder.mkdir(exist_ok=True)
        temp = folder / f"{action}.tmp"
        temp.write_text(json.dumps({"id": action, "action": action}))
        temp.replace(folder / f"{action}.json")

    def game_pid(self):
        # .NET can spawn the child on any of its worker threads.
        for children in Path(f"/proc/{self.process.pid}/task").glob("*/children"):
            try:
                for pid in children.read_text().split():
                    if Path(f"/proc/{pid}/comm").read_text().strip() == "partyboard":
                        return int(pid)
            except FileNotFoundError:
                continue
        raise RuntimeError("The room reports playing but has no live game process")

    def close(self):
        if self.process.poll() is None:
            self.request("close")
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(self.process.pid, signal.SIGKILL)
                self.process.wait()


def until(description, check, timeout=120):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = check()
        if result:
            return result
        time.sleep(0.1)
    raise TimeoutError(description)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("companion", type=Path)
    parser.add_argument("disc", type=Path)
    parser.add_argument("--players", type=int, choices=(2, 3, 4), default=2)
    args = parser.parse_args()
    companion, disc = args.companion.resolve(strict=True), args.disc.resolve(strict=True)
    seats = []
    with tempfile.TemporaryDirectory(prefix="partyboard-online-") as workspace:
        root = Path(workspace)
        try:
            host = Seat(0, root, companion, disc, args.players, None)
            seats.append(host)
            invite = until("host invitation", lambda: host.state().get("invitation"))
            for index in range(1, args.players):
                seats.append(Seat(index, root, companion, disc, args.players, invite))
            until("verified profiles and UDP routes", lambda: host.state().get("canStart"))
            host.request("start")
            until("all games released by the start barrier",
                  lambda: all(seat.state().get("state") == "playing" for seat in seats))
            games = [seat.game_pid() for seat in seats]
            host.request("close")
            until("all game processes closed", lambda: all(not Path(f"/proc/{p}").exists() for p in games), 30)
            for seat in seats:
                if seat.process.wait(timeout=10) != 0 or seat.state()["state"] != "closed":
                    raise RuntimeError("A companion did not close cleanly")
            print(f"PASS: {args.players} verified seats, TLS lobby, UDP, native start and shutdown")
        finally:
            for seat in seats:
                seat.close()


if __name__ == "__main__":
    main()
