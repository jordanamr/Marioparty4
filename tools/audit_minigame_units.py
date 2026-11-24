#!/usr/bin/env python3
"""Compile and analyze every minigame translation unit and index diagnostics by overlay.

Run from a Linux checkout with a configured Ninja build:
  python3 tools/audit_minigame_units.py build/linux-clang-relwithdebinfo --out /tmp/party-audit
The JSON report and per-unit logs are source/compiler evidence. Gameplay and
network results belong to the separate two-engine campaign.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
UNIT = re.compile(r"src/REL/(m\d{3}dll)/[^/]+\.c$", re.I)
DIAGNOSTIC = re.compile(r"^(.*?):(\d+):(\d+): (warning|error|fatal error): (.+?)(?: \[([^\]]+)\])?$")
COMMON = ["-Wreturn-type", "-Warray-bounds", "-Wuninitialized", "-Wshift-count-overflow",
          "-Wint-to-pointer-cast", "-Wpointer-to-int-cast", "-Wstrict-aliasing"]
MODES = {
    "clang": ["clang", *COMMON, "-Wconditional-uninitialized", "-Wunsequenced", "-Wloop-analysis"],
    "gcc": ["gcc", *COMMON, "-Wmaybe-uninitialized", "-Waggressive-loop-optimizations",
            "-Wsequence-point", "-Wstringop-overflow=2", "-Wnull-dereference"],
    "clang-analyzer": ["clang", "--analyze", "-Xanalyzer", "-analyzer-output=text",
                       "-Xanalyzer", "-analyzer-checker=security.ArrayBound",
                       "-Xanalyzer", "-analyzer-disable-checker=deadcode.DeadStores"],
}


def unit_rules(build):
    db = json.loads(subprocess.check_output(["ninja", "-C", str(build), "-t", "compdb"], text=True))
    rules = {}
    for row in db:
        file = Path(row["file"])
        if not file.is_absolute():
            file = (Path(row["directory"]) / file).resolve()
        rel = file.relative_to(ROOT).as_posix() if file.is_relative_to(ROOT) else ""
        if not UNIT.fullmatch(rel):
            continue
        args = iter(shlex.split(row["command"])[1:])
        flags = []
        for arg in args:
            if arg in ("-o", "-MF", "-MT", "-MQ", "-c"):
                next(args)
            elif arg not in ("-MD", "-MMD", "-g", "-w"):
                flags.append(arg)
        rules[rel] = flags
    sources = {p.relative_to(ROOT).as_posix() for p in (ROOT / "src/REL").glob("*/*.c")
               if UNIT.fullmatch(p.relative_to(ROOT).as_posix())}
    if set(rules) != sources or not sources:
        raise ValueError(f"source/Ninja inventory differs: {sorted(sources ^ set(rules))}")
    return rules


def inspect_unit(build, output, rel, flags, mode):
    path = Path(rel)
    log = output / mode / path.parent.name / (path.name + ".log")
    log.parent.mkdir(parents=True, exist_ok=True)
    math_imports = []
    with tempfile.TemporaryDirectory() as tmp:
        command = [*MODES[mode], *flags]
        command += [str(ROOT / rel)] if mode == "clang-analyzer" else [
            "-c", str(ROOT / rel), "-o", str(Path(tmp) / "unit.o")]
        process = subprocess.run(command, cwd=build, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, text=True, errors="replace")
        if mode != "clang-analyzer" and process.returncode == 0:
            symbols = subprocess.check_output(["nm", "-u", str(Path(tmp) / "unit.o")], text=True)
            # Actual unresolved library calls after compiler builtins and game math wrappers.
            runtime_math = re.compile(r"(?:sin|cos|tan|atan2?|acos|asin|exp|log|sqrt|pow|fmod|remainder|hypot|"
                                      r"ceil|floor|round|nearbyint|trunc|sincos)f?|rand|srand|clock|time")
            math_imports = sorted({line.split()[-1] for line in symbols.splitlines()
                                   if line.split() and runtime_math.fullmatch(line.split()[-1])})
    log.write_text(process.stdout)
    findings = []
    for line in process.stdout.splitlines():
        match = DIAGNOSTIC.match(line)
        if match:
            name, line_no, col, level, message, kind = match.groups()
            if name.startswith(str(ROOT) + "/"):
                name = name[len(str(ROOT)) + 1:]
            findings.append(dict(file=name, line=int(line_no), column=int(col),
                                 severity=level, message=message, category=kind))
    return dict(mode=mode, unit=path.name, exit_code=process.returncode,
                log=str(log.relative_to(output)), diagnostics=findings, runtime_math_or_clock_imports=math_imports)


def main():
    cli = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    cli.add_argument("build", type=Path)
    cli.add_argument("--out", type=Path, required=True)
    cli.add_argument("--jobs", type=int, default=6)
    cli.add_argument("--modes", nargs="+", choices=MODES, default=list(MODES))
    cli.add_argument("--cpu", action="store_true", help="run GCC and Clang CPU regressions with UBSan")
    args = cli.parse_args()
    if args.jobs < 1:
        cli.error("--jobs must be positive")
    build, output = args.build.resolve(), args.out.resolve()
    rules = unit_rules(build)
    versions = {name: subprocess.check_output([MODES[name][0], "--version"], text=True).splitlines()[0]
                for name in args.modes}
    modules = {}
    work = []
    with ThreadPoolExecutor(args.jobs) as workers:
        for rel, flags in sorted(rules.items()):
            module = Path(rel).parent.name
            record = modules.setdefault(module, {"units": {}, "checks": []})
            record["units"][Path(rel).name] = hashlib.sha256((ROOT / rel).read_bytes()).hexdigest()
            for mode in args.modes:
                work.append((record, workers.submit(inspect_unit, build, output, rel, flags, mode)))
        for record, future in work:
            record["checks"].append(future.result())
    for record in modules.values():
        record["compiler_status"] = "error" if any(t["exit_code"] for t in record["checks"]) else "completed"
        record["diagnostics"] = sum(len(t["diagnostics"]) for t in record["checks"])
    cpu_runs = []
    for record in modules.values():
        record["cpu_tests"] = []
    if args.cpu:
        for cc in ("clang", "gcc"):
            cpu_path = output / f"{cc}-cpu.json"
            cpu_log = output / f"{cc}-cpu.log"
            cmd = [sys.executable, str(ROOT / "tools/test_minigame_determinism.py"), str(build),
                   "--compiler", cc, "--ubsan", "--report", str(cpu_path), "--jobs", str(args.jobs)]
            with cpu_log.open("w") as log:
                result = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT)
            cpu_runs.append({"compiler": cc, "exit_code": result.returncode, "report": cpu_path.name,
                             "log": cpu_log.name})
            for test in json.loads(cpu_path.read_text())["tests"]:
                if test["module"] in modules:
                    modules[test["module"]]["cpu_tests"].append(dict(compiler=cc, ubsan=True, **test))
    report = {"schema": 1, "cpu_runs": cpu_runs, "evidence": "source analysis and isolated CPU functions",
              "full_matches_executed_by_this_tool": 0, "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT,
              text=True).strip(), "toolchains": versions, "modules": modules}
    (output / "audit.json").write_text(json.dumps(report, indent=2) + "\n")
    errors = sum(x["compiler_status"] == "error" for x in modules.values())
    print(f"{len(modules)} overlays, {len(rules)} translation units, {len(work)} compiler invocations, "
          f"{errors} overlay(s) with compiler failures; report {output}/audit.json")
    return bool(errors or any(c["exit_code"] for c in cpu_runs))


if __name__ == "__main__":
    raise SystemExit(main())
