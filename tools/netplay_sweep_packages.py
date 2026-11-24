"""Prepare the two published Party Board packages for headless netplay campaigns."""

import fcntl
import hashlib
import os
from pathlib import Path, PurePosixPath, PureWindowsPath
import shutil
import subprocess
import zipfile


def _digest(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _validate(root, linux):
    binary = root / "usr/bin" if linux else root
    libraries = root / "usr/lib" if linux else root
    if linux:
        programs = ("partyboard", "PartyBoardOnline")
        dependencies = ("libdol.so", "librmlui.so", "libpng16.so.16", "libzstd.so.1")
    else:
        programs = ("partyboard.exe", "PartyBoardOnline.exe", "PartyBoardOnline.exe.config")
        dependencies = ("dol.dll", "SDL3.dll", "nod.dll", "rmlui.dll", "webgpu_dawn.dll",
                        "dxcompiler.dll", "dxil.dll", "zlib1.dll", "libpng16.dll")
    required = [binary / name for name in (*programs, "res/icon.png")]
    required.extend(libraries / name for name in dependencies)
    suffix = ".so" if linux else ".dll"
    overlays = ["bootDll", "modeseldll", "mentDll", "messDll", "instDll", "_minigameDLL",
                "mgmodedll", "mpexDll", "mstoryDll", "mstory2Dll", "mstory3Dll", "mstory4Dll",
                "option", "present", "resultDll", "selmenuDll", "staffDll", "subchrselDll", "ztardll"]
    overlays.extend(f"w{n:02d}Dll" for n in (1, 2, 3, 4, 5, 6, 10, 20, 21))
    for n in range(401, 464):
        if n in (452, 454):  # No game overlay was released at these two IDs.
            continue
        # These module names have a lower-case suffix in the original game.
        overlays.append(f"m{n}dll" if n in (407, 420, 444, 447, 459) else f"m{n}Dll")
    required.extend(libraries / (name + suffix) for name in overlays)
    if linux:
        required.append(binary / "libdol.so")
    for file in required:
        if not file.is_file() or not file.stat().st_size:
            raise ValueError(f"Incomplete package: missing or empty {file.relative_to(root)}")
    if linux:
        for name in programs:
            if not os.access(binary / name, os.X_OK):
                raise ValueError(f"Linux package is missing execute permission on {name}")
    return binary


def _extract(source, work, linux):
    if linux:
        with source.open("rb") as stream:
            header = stream.read(11)
        if header[:4] != b"\x7fELF" or header[8:] != b"AI\x02":
            raise ValueError(f"Expected a type 2 AppImage: {source}")
        executable = source
        if not os.access(source, os.X_OK):
            executable = work / "extract.AppImage"
            shutil.copy2(source, executable)
            executable.chmod(executable.stat().st_mode | 0o100)
        result = subprocess.run([executable, "--appimage-extract"], cwd=work, stdout=subprocess.DEVNULL)
        if result.returncode:
            raise RuntimeError(f"AppImage extraction failed ({result.returncode}): {source}")
        return work / "squashfs-root"

    target = work / "windows"
    with zipfile.ZipFile(source) as archive:
        for name in archive.namelist():
            if (PureWindowsPath(name).anchor or "\\" in name or
                    ".." in PurePosixPath(name).parts):
                raise ValueError(f"Unexpected ZIP member path: {name!r}")
        archive.extractall(target)
    if (target / "partyboard.exe").is_file():
        return target
    # A CI artifact or unzip/repack may add a single top-level folder.
    children = list(target.glob("*/partyboard.exe"))
    if len(children) == 1:
        return children[0].parent
    raise ValueError(f"Expected a Windows package containing partyboard.exe: {source}")


def _prepare(source, cache, linux):
    source = Path(source).resolve(strict=True)
    if not source.is_file():
        raise ValueError(f"Expected a package file: {source}")
    key = ("linux-" if linux else "windows-") + _digest(source)
    destination = cache / key
    work = cache / (key + ".partial")
    with (cache / (key + ".lock")).open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if work.exists():
            shutil.rmtree(work)
        if destination.exists():
            try:
                return _validate(destination, linux)
            except ValueError:
                shutil.rmtree(destination)
        work.mkdir()
        try:
            try:
                extracted = _extract(source, work, linux)
            except zipfile.BadZipFile as exc:
                raise ValueError(f"Invalid Windows ZIP: {source}: {exc}") from exc
            binary = _validate(extracted, linux)
            relative = binary.relative_to(extracted)
            extracted.rename(destination)
            return destination / relative
        finally:
            shutil.rmtree(work)


def prepare_packages(appimage: Path, windows_zip: Path, cache_dir: Path) -> tuple[Path, Path]:
    """Return (Linux executable directory, Windows executable directory), using a content cache."""
    cache = Path(cache_dir).resolve()
    cache.mkdir(parents=True, exist_ok=True)
    return _prepare(appimage, cache, True), _prepare(windows_zip, cache, False)
