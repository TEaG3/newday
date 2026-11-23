#!/usr/bin/env python3
"""Package the final Arabic, single-entrypoint TEa.G3 PS2 release."""
from __future__ import annotations
import hashlib
import os
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
NAME = "TEa.G3-PS2"
ZIP_PATH = DIST / f"{NAME}.zip"
VERSION = sys.argv[1] if len(sys.argv) > 1 else "development"

REQUIRED = (
    "OPL.ELF",
    "neutrino/neutrino.elf",
    "neutrino/modules/ee_core.elf",
    "neutrino/modules/raagent.irx",
    "neutrino/modules/cdvdman_emu.irx",
    "neutrino/modules/udpfs_fhi.irx",
    "neutrino/modules/smap.irx",
    "neutrino/modules/ministack.irx",
    "neutrino/modules/smb_ioman.irx",
    "neutrino/modules/smb_fhi.irx",
    "neutrino/modules/mmcefhi.irx",
)

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()

def main() -> None:
    elf = ROOT / "TEa.G3.ELF"
    manifest = ROOT / "runtime-manifest.txt"
    opl_license = ROOT / "OPL-LICENSE.txt"
    neutrino_license = ROOT / "third_party/neutrino-igr/LICENSE"
    required_files = (elf, manifest, ROOT / "INSTALL-AR.txt", ROOT / "LICENSE", opl_license, neutrino_license)
    missing = [str(p.relative_to(ROOT)) for p in required_files if not p.is_file() or p.stat().st_size == 0]
    if missing:
        raise SystemExit("release inputs missing or empty: " + ", ".join(missing))
    embedded = manifest.read_text(encoding="utf-8")
    for item in REQUIRED:
        if item not in embedded:
            raise SystemExit("embedded runtime manifest is incomplete: " + item)

    DIST.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(ZIP_PATH, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.write(elf, "TEa.G3.ELF")
        z.write(ROOT / "INSTALL-AR.txt", "INSTALL-AR.txt")
        z.write(ROOT / "LICENSE", "LICENSE")
        z.write(opl_license, "licenses/OPL-GPL-3.0.txt")
        z.write(neutrino_license, "licenses/Neutrino-AFL-3.0.txt")
        z.writestr("QUICKSTART.txt", (ROOT / "INSTALL-AR.txt").read_text(encoding="utf-8"))
        z.writestr("DVD/ضع ألعاب DVD بصيغة ISO أو ZSO هنا.txt", "ضع ألعاب PS2 DVD هنا.\n")
        z.writestr("CD/ضع ألعاب CD هنا.txt", "ضع ألعاب PS2 CD هنا.\n")
        z.writestr("docs/runtime-manifest.txt", manifest.read_text(encoding="utf-8"))

    sums = DIST / "SHA256SUMS.txt"
    sums.write_text(
        f"{sha256(ZIP_PATH)}  {ZIP_PATH.name}\n"
        f"{sha256(elf)}  {elf.name}\n",
        encoding="utf-8",
    )
    print(f"version={VERSION}")
    print(f"package={ZIP_PATH} ({ZIP_PATH.stat().st_size} bytes)")
    print(f"elf_sha256={sha256(elf)}")
    print(f"zip_sha256={sha256(ZIP_PATH)}")

if __name__ == "__main__":
    main()
