#!/usr/bin/env python3
"""Build a single-ELF OPL-only TEa.G3 user package."""
from __future__ import annotations
import hashlib
import os
import re
import time
from pathlib import Path
import sys
import zipfile

try:
    from tools.verify_runtime_manifest import verify_runtime_manifest
except ModuleNotFoundError as exc:
    if exc.name != "tools":
        raise
    # Direct script execution puts tools/ (not the repository root) on sys.path.
    from verify_runtime_manifest import verify_runtime_manifest

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
NAME = "TEa.G3-Complete-Package"
ZIP_PATH = DIST / f"{NAME}.zip"
VERSION = sys.argv[1] if len(sys.argv) > 1 else "development"
REQUIRED = ("OPL.ELF",)
OPL_LICENSE_ARCHIVE_PATH = "licenses/OPL-AFL-3.0.txt"

def validate_opl_license(text: str) -> str:
    if 'Academic Free License ("AFL") v. 3.0' not in text[:512]:
        raise ValueError("OPL-LICENSE.txt does not declare the expected AFL-3.0 license")
    return OPL_LICENSE_ARCHIVE_PATH

def validate_release_version(version: str, launcher_source: str) -> str:
    if version == "development":
        return version
    tag_version = version[len("arabic-"):] if version.startswith("arabic-") else version
    if not re.fullmatch(r"v\d+(?:\.\d+){1,3}", tag_version):
        raise ValueError(f"invalid TEa.G3 release version: {version}")
    match = re.search(r'#define TEAG3_VERSION "([^"]+)"', launcher_source)
    if not match:
        raise ValueError("launcher.c does not define TEAG3_VERSION")
    if match.group(1) != tag_version:
        raise ValueError(f"launcher.c TEAG3_VERSION is {match.group(1)}, not {tag_version}")
    return tag_version

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()

def archive_timestamp() -> tuple[int, int, int, int, int, int]:
    """Return a ZIP-safe UTC timestamp derived only from SOURCE_DATE_EPOCH."""
    raw_epoch = os.environ.get("SOURCE_DATE_EPOCH", "315532800")
    try:
        epoch = int(raw_epoch)
    except ValueError as exc:
        raise ValueError("SOURCE_DATE_EPOCH must be an integer Unix timestamp") from exc
    epoch = max(epoch, 315532800)  # ZIP cannot represent dates before 1980-01-01.
    try:
        stamp = time.gmtime(epoch)
    except (OverflowError, OSError, ValueError) as exc:
        raise ValueError("SOURCE_DATE_EPOCH is outside the supported timestamp range") from exc
    if stamp.tm_year > 2107:
        raise ValueError("SOURCE_DATE_EPOCH is outside the ZIP timestamp range")
    return (stamp.tm_year, stamp.tm_mon, stamp.tm_mday, stamp.tm_hour,
            stamp.tm_min, stamp.tm_sec - (stamp.tm_sec % 2))

def _archive_info(name: str) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, date_time=archive_timestamp())
    info.compress_type = zipfile.ZIP_DEFLATED
    info.create_system = 3
    info.external_attr = 0o100644 << 16
    return info

def add_archive_file(z: zipfile.ZipFile, src: Path, dest: str) -> None:
    z.writestr(_archive_info(dest), src.read_bytes(),
               compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)

def add_archive_text(z: zipfile.ZipFile, dest: str, text: str) -> None:
    z.writestr(_archive_info(dest), text.encode("utf-8"),
               compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)

def main() -> None:
    try:
        validate_release_version(VERSION, (ROOT / "launcher.c").read_text(encoding="utf-8"))
    except ValueError as exc:
        raise SystemExit(str(exc))
    elf, manifest = ROOT / "TEa.G3.ELF", ROOT / "runtime-manifest.txt"
    opl_license, opl_diagnostic = ROOT / "OPL-LICENSE.txt", ROOT / "runtime-input/OPL.ELF"
    required_files = (elf, manifest, ROOT / "INSTALL-AR.txt", ROOT / "INSTALL-EN.txt",
                      ROOT / "README.md", ROOT / "README-EN.md", ROOT / "LICENSE", opl_license)
    missing = [str(p.relative_to(ROOT)) for p in required_files if not p.is_file() or p.stat().st_size == 0]
    if missing:
        raise SystemExit("release inputs missing or empty: " + ", ".join(missing))
    try:
        license_path = validate_opl_license(opl_license.read_text(encoding="utf-8"))
    except ValueError as exc:
        raise SystemExit(str(exc))
    try:
        bundle_id, asset_count = verify_runtime_manifest(ROOT / "runtime-input", manifest)
    except (OSError, UnicodeError, ValueError) as exc:
        raise SystemExit(f"embedded OPL runtime manifest is invalid: {exc}")
    if asset_count != len(REQUIRED):
        raise SystemExit(f"embedded OPL runtime has {asset_count} assets, expected {len(REQUIRED)}")
    DIST.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(ZIP_PATH, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for src, dest in ((elf, "TEa.G3.ELF"), (ROOT / "INSTALL-AR.txt", "INSTALL-AR.txt"),
                          (ROOT / "INSTALL-EN.txt", "INSTALL-EN.txt"), (ROOT / "README.md", "README-AR.md"),
                          (ROOT / "README-EN.md", "README-EN.md"), (ROOT / "LICENSE", "LICENSE"),
                          (opl_license, license_path)):
            add_archive_file(z, src, dest)
        if opl_diagnostic.is_file():
            add_archive_file(z, opl_diagnostic, "diagnostics/OPL-standalone.ELF")
        add_archive_text(z, "QUICKSTART.txt", (ROOT / "INSTALL-AR.txt").read_text(encoding="utf-8"))
        add_archive_text(z, "DVD/ضع ألعاب DVD بصيغة ISO أو ZSO هنا.txt", "ضع ألعاب DVD هنا.\n")
        add_archive_text(z, "CD/ضع ألعاب CD بصيغة ISO أو ZSO هنا.txt", "ضع ألعاب CD هنا.\n")
        add_archive_text(z, "POPS/ضع ألعاب PS1 المتوافقة هنا.txt", "ضع ملفات PS1 المتوافقة مع POPStarter هنا.\n")
        add_archive_text(z, "APPS/ضع تطبيقات ELF هنا.txt", "ضع تطبيقات Homebrew بصيغة ELF هنا.\n")
        add_archive_text(z, "PACKAGE-MANIFEST.txt",
            f"TEa.G3 OPL-only package\nVersion: {VERSION}\n"
            "Single user entry point: TEa.G3.ELF\n"
            "OPL is the only PS2 game engine embedded in the launcher.\n"
            "SMB, UDPBD, UDPFS game streaming and MMCE game launching are disabled until their OPL readers pass validation.\n"
            "No game images are included.\n"
            "CI success does not replace physical PS2 acceptance testing.\n")
    sums = DIST / "SHA256SUMS.txt"
    sums.write_text(f"{sha256(ZIP_PATH)}  {ZIP_PATH.name}\n{sha256(elf)}  {elf.name}\n", encoding="utf-8")
    with zipfile.ZipFile(ZIP_PATH) as z:
        bad = z.testzip()
        required = {"TEa.G3.ELF", "INSTALL-AR.txt", "INSTALL-EN.txt", "README-AR.md", "README-EN.md",
                    "DVD/ضع ألعاب DVD بصيغة ISO أو ZSO هنا.txt", "CD/ضع ألعاب CD بصيغة ISO أو ZSO هنا.txt",
                    "POPS/ضع ألعاب PS1 المتوافقة هنا.txt", "APPS/ضع تطبيقات ELF هنا.txt"}
        missing_entries = required.difference(z.namelist())
        if bad or missing_entries:
            raise SystemExit(f"package validation failed: CRC={bad}, missing={sorted(missing_entries)}")
    print(f"version={VERSION}")
    print(f"package={ZIP_PATH} ({ZIP_PATH.stat().st_size} bytes)")
    print(f"elf_sha256={sha256(elf)}")
    print(f"zip_sha256={sha256(ZIP_PATH)}")
    print("package_crc=PASS")

if __name__ == "__main__":
    main()
