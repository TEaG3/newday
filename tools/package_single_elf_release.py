#!/usr/bin/env python3
"""Build one complete, single-ELF TEa.G3 user package for USB installation."""
from __future__ import annotations
import hashlib
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
NAME = "TEa.G3-Complete-Package"
ZIP_PATH = DIST / f"{NAME}.zip"
VERSION = sys.argv[1] if len(sys.argv) > 1 else "development"

REQUIRED = (
    "OPL.ELF", "neutrino/neutrino.elf", "neutrino/modules/ee_core.elf",
    "neutrino/modules/raagent.irx", "neutrino/modules/cdvdman_emu.irx",
    "neutrino/modules/udpfs_fhi.irx", "neutrino/modules/smap.irx",
    "neutrino/modules/ministack.irx", "neutrino/modules/smb_ioman.irx",
    "neutrino/modules/smb_fhi.irx", "neutrino/modules/mmcefhi.irx",
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
    opl_diagnostic = ROOT / "runtime-input/OPL.ELF"
    neutrino_tree = DIST / "neutrino"
    neutrino_license = ROOT / "third_party/neutrino-igr/LICENSE"
    required_files = (
        elf, manifest, ROOT / "INSTALL-AR.txt", ROOT / "INSTALL-EN.txt",
        ROOT / "README.md", ROOT / "LICENSE", opl_license, neutrino_license,
    )
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
        z.write(ROOT / "INSTALL-EN.txt", "INSTALL-EN.txt")
        z.write(ROOT / "README.md", "README.md")
        z.write(ROOT / "LICENSE", "LICENSE")
        z.write(opl_license, "licenses/OPL-GPL-3.0.txt")
        z.write(neutrino_license, "licenses/Neutrino-AFL-3.0.txt")
        # Optional diagnostics are bundled for completeness; normal installation uses the single embedded ELF.
        if opl_diagnostic.is_file():
            z.write(opl_diagnostic, "diagnostics/OPL-standalone.ELF")
        if neutrino_tree.is_dir():
            for item in sorted(neutrino_tree.rglob("*")):
                if item.is_file():
                    z.write(item, "diagnostics/neutrino/" + item.relative_to(neutrino_tree).as_posix())
        z.writestr("README-AR.txt", "حزمة TEa.G3 الشاملة. انسخ TEa.G3.ELF ومجلدات الألعاب إلى جذر USB. راجع INSTALL-AR.txt.\n")
        z.writestr("QUICKSTART.txt", (ROOT / "INSTALL-AR.txt").read_text(encoding="utf-8"))
        z.writestr("DVD/ضع ألعاب DVD بصيغة ISO أو ZSO هنا.txt", "ضع ألعاب PS2 DVD هنا.\n")
        z.writestr("CD/ضع ألعاب CD هنا.txt", "ضع ألعاب PS2 CD هنا.\n")
        z.writestr("POPS/ضع ألعاب PS1 المتوافقة هنا.txt", "ضع ملفات PS1 المتوافقة مع POPStarter هنا.\n")
        z.writestr("APPS/ضع تطبيقات ELF هنا.txt", "ضع تطبيقات Homebrew بصيغة ELF هنا.\n")
        z.writestr("USB-Template/README-AR.txt", "قالب مجلدات USB؛ ملفات UL تبقى في الجذر.\n")
        z.writestr("USB-Template/README-EN.txt", "USB folder template; UL files remain in the root.\n")
        z.writestr("docs/runtime-manifest.txt", manifest.read_text(encoding="utf-8"))
        z.writestr("PACKAGE-MANIFEST.txt",
            f"TEa.G3 complete package\nVersion: {VERSION}\n"
            "Single user entry point: TEa.G3.ELF\n"
            "OPL and Neutrino runtimes are embedded in the ELF.\n"
            "Standalone OPL/Neutrino diagnostic copies, if present, are optional and not required for installation.\n"
            "No game images are included.\n"
            "CI success does not replace physical PS2 acceptance testing.\n")

    sums = DIST / "SHA256SUMS.txt"
    sums.write_text(f"{sha256(ZIP_PATH)}  {ZIP_PATH.name}\n{sha256(elf)}  {elf.name}\n", encoding="utf-8")
    with zipfile.ZipFile(ZIP_PATH) as z:
        bad = z.testzip()
        required = {"TEa.G3.ELF", "INSTALL-AR.txt", "INSTALL-EN.txt", "DVD/ضع ألعاب DVD بصيغة ISO أو ZSO هنا.txt",
                    "CD/ضع ألعاب CD هنا.txt", "POPS/ضع ألعاب PS1 المتوافقة هنا.txt", "APPS/ضع تطبيقات ELF هنا.txt"}
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
