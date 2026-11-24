#!/usr/bin/env python3
"""Check cross-file C signatures of each separately linked minigame with GCC LTO."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import tempfile

from audit_minigame_units import ROOT, unit_rules


def inspect_module(name, units, build, output):
    log = output / (name + ".log")
    with tempfile.TemporaryDirectory() as temp, log.open("w") as stream:
        d = Path(temp)
        objects = []
        for source, flags in units:
            obj = d / (Path(source).stem + ".o")
            command = ["gcc", *flags, "-O0", "-flto", "-w", "-c", str(ROOT / source), "-o", str(obj)]
            result = subprocess.run(command, cwd=build, stdout=stream, stderr=stream)
            if result.returncode:
                return {"module": name, "status": "compiler_error", "source": source, "log": log.name}
            objects.append(str(obj))
        # A non-LTO relocatable output forces type merging during this link.
        result = subprocess.run(["gcc", "-flto", "-r", "-flinker-output=nolto-rel", "-Werror=lto-type-mismatch",
                                 *objects, "-o", str(d / "module.o")], stdout=stream, stderr=stream)
    return {"module": name, "status": "pass" if result.returncode == 0 else "link_error", "log": log.name,
            "units": [s for s, _ in units]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    build = args.build.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    modules = {}
    for source, flags in unit_rules(build).items():
        modules.setdefault(Path(source).parent.name, []).append((source, flags))
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        work = [pool.submit(inspect_module, name, units, build, args.out) for name, units in sorted(modules.items())]
        rows = [job.result() for job in work]
    bad = [r["module"] for r in rows if r["status"] != "pass"]
    (args.out / "abi.json").write_text(json.dumps(rows, indent=2) + "\n", encoding="utf-8")
    print(f"{len(rows)} modules, {sum(len(units) for units in modules.values())} translation units; failures: {bad}")
    return bool(bad)


if __name__ == "__main__":
    raise SystemExit(main())
