"""Classify evidence from one two-engine run and publish campaign results as JSON."""
from __future__ import annotations

from collections import Counter
import argparse
from pathlib import Path
import json
import re
import tempfile

from netplay_compare import first_field_difference, last_matching_frame, parse

PAIRS = re.compile(r"\b(\w+)=([^\s]+)")
NEW_OVERLAY = re.compile(r"Start New OVL (\d+)\b")
CONTEXT = re.compile(r"game context (-?\d+) at network frame (\d+)")
DIAGNOSTIC = re.compile(r"\b(DESYNC|PROTOCOL) frame=(\d+)\b")
HASH = re.compile(r"[0-9a-f]{16}", re.I)


def _read(path):
    return path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""


def _route(log, module, results_contexts, direct_return=False):
    loaded = re.compile(rf"\b{re.escape(module)}\.(?:so|dll) prolog end\b", re.I)
    unloaded = re.compile(rf"(?:End|Unlink) DLL:\s*{re.escape(module)}\.(?:so|dll)\b", re.I)
    route = {"entered": False, "context": None, "exited": False, "result_overlay": None,
             "after_result_frame": None, "after_result_context": None}
    current = None
    exited = False
    for line in log.splitlines():
        transition = NEW_OVERLAY.search(line)
        if transition:
            current = int(transition[1])
            if exited:
                exited = False
                if current in results_contexts:
                    route["result_overlay"] = current
        if loaded.search(line):
            route["entered"] = True
            route["context"] = current
        if route["entered"] and unloaded.search(line):
            route["exited"] = True
            exited = True
        tick = CONTEXT.search(line)
        at_destination = current == route["result_overlay"] if direct_return else current not in results_contexts
        if (tick and route["result_overlay"] is not None and at_destination
                and int(tick[1]) == current and current >= 0):
            route["after_result_frame"] = int(tick[2])
            route["after_result_context"] = int(tick[1])
            break
    return route


def _validated_report_pairs(reports, peers, frame):
    selected = [next((r for r in reports[p] if r.first_desync_frame == frame), None) for p in peers]
    if not all(selected):
        return None
    left, right = selected
    for key in ("session", "protocol", "hash_version", "disc", "disc_version", "build_revision"):
        if left.header.get(key) != right.header.get(key):
            return None
    return selected


def classify_case(run_dir, *, overlay, peers=("linux", "win"), timed_out=False, exit_codes=None,
                  board_id=None, min_turn=2, context=None, results_contexts=(84,), direct_return=False):
    """Compare two peers. A board PASS covers min_turn, a minigame PASS covers results."""
    directory = Path(run_dir).resolve()
    peers = tuple(peers)
    if len(peers) != 2 or peers[0] == peers[1]:
        raise ValueError("supply two distinct peer names")
    module = str(overlay)
    if module.isdecimal():
        module = f"m{module}dll"
    checkpoints, routes, side_players, failures = {}, {}, {}, []
    reports = {p: [] for p in peers}
    evidence = {p: [] for p in peers}
    unreadable_reports = []
    for peer in peers:
        path = directory / f"{peer}.log"
        log = _read(path)
        native = directory / f"{peer}.native"
        sidecar = directory / f"{peer}.native.desync"
        evidence[peer] = [p.name for p in (path, native, sidecar) if p.is_file()]
        routes[peer] = _route(log, module, results_contexts, direct_return)
        checkpoints[peer] = {}
        for line in (_read(native) + "\n" + _read(sidecar) + "\n" + log).splitlines():
            if "event=checkpoint " in line:
                data = dict(PAIRS.findall(line))
                if data.get("hash_frame", "").isdigit() and HASH.fullmatch(data.get("state_hash", "")):
                    checkpoints[peer][int(data["hash_frame"])] = data
                    side_players[peer] = data.get("player")
            match = DIAGNOSTIC.search(line)
            if match:
                failures.append((int(match[2]), match[1], peer, dict(PAIRS.findall(line))))
    for path in sorted(directory.rglob("netplay_desync*.log")):
        try:
            report = parse(str(path))
            reported_frame = report.first_desync_frame
        except (OSError, ValueError):
            # The supervisor may sample while a peer is still writing the report.
            unreadable_reports.append(str(path.relative_to(directory)))
            continue
        if reported_frame is None:
            continue
        peer = next((p for p in peers if side_players.get(p) == report.player), None)
        if peer is None:
            top = path.relative_to(directory).parts[0]
            peer = next((p for p in peers if top in (p, f"{p}-profile")), None)
        if peer is not None:
            reports[peer].append(report)
            evidence[peer].append(str(path.relative_to(directory)))
            failures.append((report.first_desync_frame, "DESYNC", peer, report.header))

    left, right = (checkpoints[p] for p in peers)
    common = sorted(left.keys() & right.keys())
    for frame in common:
        if left[frame].get("hash_version") != right[frame].get("hash_version"):
            failures.append((frame, "PROTOCOL", peers[0], {"category": "hash_version"}))
            break
        if left[frame]["state_hash"] != right[frame]["state_hash"]:
            failures.append((frame, "DESYNC", peers[0], {}))
            break
    first = min((f[0] for f in failures), default=None)
    equal = [f for f in common if (first is None or f < first) and left[f]["state_hash"] == right[f]["state_hash"]]
    last = equal[-1] if equal else None
    result = {"status": "INCOMPLETE", "overlay": module, "run_dir": str(directory), "peers": list(peers),
              "route": routes, "milestone": "board_turn" if board_id is not None else "minigame_results",
              "board_id": board_id, "min_turn": min_turn if board_id is not None else None,
              "matching_checkpoints": len(equal), "last_matching_checkpoint": last,
              "last_matching_hash": left[last]["state_hash"] if last is not None else None,
              "last_equal_frame": last, "first_divergence": None, "evidence": evidence,
              "exit_codes": exit_codes or {}, "timed_out": timed_out, "unreadable_reports": unreadable_reports}
    if direct_return:
        result["milestone"] = "minigame_direct_return"

    if first is not None:
        observations = [(kind, p, data) for f, kind, p, data in failures if f == first]
        kind = "PROTOCOL" if any(k == "PROTOCOL" for k, _, _ in observations) else "DESYNC"
        category = next((d["category"] for _, _, d in observations if "category" in d), kind)
        hashes = {p: checkpoints[p][first]["state_hash"] for p in peers if first in checkpoints[p]}
        for _, p, data in observations:
            other = peers[1] if p == peers[0] else peers[0]
            for side, key in ((p, "localHash"), (other, "remoteHash")):
                if HASH.fullmatch(data.get(key, "")):
                    hashes.setdefault(side, data[key])
        field = None
        selected = _validated_report_pairs(reports, peers, first)
        if selected:
            previous = last_matching_frame(*selected)
            if previous is not None and previous < first:
                result["last_equal_frame"] = max(result["last_equal_frame"] or 0, previous)
            for p, report in zip(peers, selected):
                digest = report.digests.get(("local", first), {}).get("hash", "")
                if HASH.fullmatch(digest):
                    hashes[p] = digest
            if all(report.field_frame == first for report in selected):
                diff = first_field_difference(*selected)
                if diff:
                    index, subsystem, name, a, b = diff
                    field = {"index": index, "subsystem": subsystem, "name": name, "values": dict(zip(peers, (a, b)))}
        # A peer's own retained report can establish the exact last good frame
        # when the other report is unavailable. Sparse checkpoints alone cannot.
        reported_good = [int(d["last_good_frame"]) for _, _, d in observations
                         if d.get("last_good_frame", "").isdigit() and int(d["last_good_frame"]) < first]
        if reported_good:
            result["last_equal_frame"] = max(result["last_equal_frame"] or 0, min(reported_good))
        result.update(status="DESYNC" if kind == "DESYNC" else "INCOMPLETE", reason=kind,
                      first_divergence={"frame": first, "category": category, "hashes": hashes, "field": field})
        return result

    if any(rc not in (0, None) for rc in result["exit_codes"].values()):
        result.update(status="CRASH", reason="abnormal_exit")
        return result
    reached = all(route["entered"] for route in routes.values())
    if board_id is not None:
        # A frozen display or a different board must never earn turn coverage.
        if context is None:
            context = routes[peers[0]]["context"]
        verified = next((f for f in equal if reached and context is not None and all(
            cp.get("context") == str(context) and cp.get("board") == str(board_id)
            and int(cp.get("turn", -1)) >= min_turn for cp in (left[f], right[f]))), None)
    else:
        marks = [r["after_result_frame"] for r in routes.values()]
        verified = None
        if all(v is not None for v in marks) and all(any(
            checkpoints[p][f].get("context") == str(routes[p]["context"]) for f in equal) for p in peers):
            verified = next((f for f in equal if f >= max(marks) and all(
                checkpoints[p][f].get("context") == str(routes[p]["after_result_context"]) for p in peers)), None)
    if verified is not None:
        result.update(status="PASS", reason=result["milestone"], verified_frame=verified)
    else:
        result.update(status="TIMEOUT" if timed_out else "INCOMPLETE",
                      reason="overlay_only" if reached else "target_not_reached")
    return result


def write_campaign(output_dir, cases):
    """Publish an atomic, stable JSON manifest. Each row retains its first failure."""
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)
    rows = sorted(cases, key=lambda c: (str(c.get("id", "")), str(c.get("run_dir", ""))))
    manifest = {"schema_version": 1, "counts": dict(sorted(Counter(c["status"] for c in rows).items())), "cases": rows}
    path = out / "manifest.json"
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=out, delete=False) as stream:
        json.dump(manifest, stream, indent=2, ensure_ascii=False)
        stream.write("\n")
    Path(stream.name).replace(path)
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("--overlay", required=True, help="Module, e.g. 407 or w05Dll")
    parser.add_argument("--peers", nargs=2, default=("linux", "win"), metavar=("PEER_A", "PEER_B"))
    parser.add_argument("--board-id", type=int)
    parser.add_argument("--min-turn", type=int, default=2)
    parser.add_argument("--context", type=int)
    parser.add_argument("--timed-out", action="store_true")
    args = parser.parse_args()
    result = classify_case(**vars(args))
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
