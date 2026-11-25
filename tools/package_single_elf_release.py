#!/usr/bin/env python3
"""Build one complete, single-ELF TEa.G3 user package for USB installation."""
from __future__ import annotations
import hashlib
import re
from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
NAME = "TEa.G3-Complete-Package"
ZIP_PATH = DIST / f"{NAME}.zip"
VERSION = sys.argv[1] if len(sys.argv) > 1 else "development"

REQUIRED = (
    "OPL.ELF",
    "network/modules/smap.irx",
    "network/modules/ministack.irx",
    "network/modules/smb_ioman.irx",
)
OPL_LICENSE_ARCHIVE_PATH = "licenses/OPL-AFL-3.0.txt"
SMB_LICENSE_ARCHIVE_PATH = "licenses/SMB-support-AFL-3.0.txt"
AFL_NOTICE = 'Academic Free License ("AFL") v. 3.0'


def validate_opl_license(license_text: str) -> str:
    """Keep the release archive label aligned with OPL's upstream license notice."""
    if AFL_NOTICE not in license_text[:512]:
        raise ValueError("OPL-LICENSE.txt does not declare the expected AFL-3.0 license")
    return OPL_LICENSE_ARCHIVE_PATH


def validate_smb_license(license_text: str) -> str:
    """Retain the upstream license for the SMB IOP support modules."""
    if AFL_NOTICE not in license_text[:512]:
        raise ValueError("SMB-LICENSE.txt does not declare the expected AFL-3.0 license")
    return SMB_LICENSE_ARCHIVE_PATH


def validate_release_version(version: str, launcher_source: str) -> str:
    """Require release metadata and the in-app updater version to identify the same build."""
    if version == "development":
        return version
    tag_version = version[len("arabic-"):] if version.startswith("arabic-") else version
    if not re.fullmatch(r"v\d+(?:\.\d+){1,3}", tag_version):
        raise ValueError(f"invalid TEa.G3 release version: {version}")
    match = re.search(r'#define TEAG3_VERSION "([^"]+)"', launcher_source)
    if not match:
        raise ValueError("launcher.c does not define TEAG3_VERSION")
    built = match.group(1)
    if built != tag_version:
        raise ValueError(f"launcher.c TEAG3_VERSION is {built}, not {tag_version}")
    return tag_version


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def main() -> None:
    try:
        validate_release_version(VERSION, (ROOT / "launcher.c").read_text(encoding="utf-8"))
    except ValueError as exc:
        raise SystemExit(str(exc))

    elf = ROOT / "TEa.G3.ELF"
    manifest = ROOT / "runtime-manifest.txt"
    opl_license = ROOT / "runtime-input/OPL-LICENSE.txt"
    smb_license = ROOT / "runtime-input/network/SMB-LICENSE.txt"
    opl_diagnostic = ROOT / "runtime-input/OPL.ELF"
    required_files = (
        elf, manifest, ROOT / "INSTALL-AR.txt", ROOT / "INSTALL-EN.txt",
        ROOT / "README.md", ROOT / "LICENSE", opl_license, smb_license,
    )
    missing = [str(p.relative_to(ROOT)) for p in required_files if not p.is_file() or p.stat().st_size == 0]
    if missing:
        raise SystemExit("release inputs missing or empty: " + ", ".join(missing))

    try:
        opl_license_archive_path = validate_opl_license(opl_license.read_text(encoding="utf-8"))
        smb_license_archive_path = validate_smb_license(smb_license.read_text(encoding="utf-8"))
    except ValueError as exc:
        raise SystemExit(str(exc))

    embedded = manifest.read_text(encoding="utf-8")
    for item in REQUIRED:
        if item not in embedded:
            raise SystemExit("embedded runtime manifest is incomplete: " + item)
    if "neutrino/neutrino.elf" in embedded.lower():
        raise SystemExit("Neutrino game runtime must not be embedded in the OPL-only package")

    DIST.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(ZIP_PATH, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.write(elf, "TEa.G3.ELF")
        z.write(ROOT / "INSTALL-AR.txt", "INSTALL-AR.txt")
        z.write(ROOT / "INSTALL-EN.txt", "INSTALL-EN.txt")
        z.write(ROOT / "README.md", "README.md")
        z.write(ROOT / "LICENSE", "LICENSE")
        z.write(opl_license, opl_license_archive_path)
        z.write(smb_license, smb_license_archive_path)
        # The standalone OPL ELF is diagnostic only; normal installation uses TEa.G3.ELF.
        if opl_diagnostic.is_file():
            z.write(opl_diagnostic, "diagnostics/OPL-standalone.ELF")
        z.writestr("README-AR.txt", "حزمة TEa.G3 الشاملة. انسخ TEa.G3.ELF ومجلدات الألعاب إلى جذر USB. راجع INSTALL-AR.txt.\n")
        z.writestr("QUICKSTART.txt", (ROOT / "INSTALL-AR.txt").read_text(encoding="utf-8"))
        z.writestr("DVD/ضع ألعاب DVD بصيغة ISO أو ZSO هنا.txt", "ضع ألعاب PS2 DVD هنا.\n")
        z.writestr("CD/ضع ألعاب CD هنا.txt", "ضع ألعاب PS2 CD هنا.\n")
        z.writestr("POPS/ضع ألعاب PS1 المتوافقة هنا.txt", "ضع ملفات PS1 المتوافقة مع POPStarter هنا.\n")
        z.writestr("APPS/ضع تطبيقات ELF هنا.txt", "ضع تطبيقات Homebrew بصيغة ELF هنا.\n")
        z.writestr("USB-Template/README-AR.txt", "قالب مجلدات USB؛ ملفات UL تبقى في الجذر.\n")
        z.writestr("USB-Template/README-EN.txt", "USB folder template; UL files remain in the root.\n")
        z.writestr("docs/runtime-manifest.txt", embedded)
        z.writestr("PACKAGE-MANIFEST.txt",
            f"TEa.G3 complete package\nVersion: {VERSION}\n"
            "Single user entry point: TEa.G3.ELF\n"
            "OPL is the only PS2 game engine.\n"
            "SMB IOP modules are embedded for catalog access; OPL ETH handles SMB game data.\n"
            "No Neutrino game ELF or Neutrino launch fallback is included.\n"
            "No game images are included.\n"
            "CI success does not replace physical PS2 acceptance testing.\n")

    sums = DIST / "SHA256SUMS.txt"
    sums.write_text(f"{sha256(ZIP_PATH)}  {ZIP_PATH.name}\n{sha256(elf)}  {elf.name}\n", encoding="utf-8")
    with zipfile.ZipFile(ZIP_PATH) as z:
        bad = z.testzip()
        required = {"TEa.G3.ELF", "INSTALL-AR.txt", "INSTALL-EN.txt",
                    "licenses/OPL-AFL-3.0.txt", "licenses/SMB-support-AFL-3.0.txt",
                    "DVD/ضع ألعاب DVD بصيغة ISO أو ZSO هنا.txt",
                    "CD/ضع ألعاب CD هنا.txt", "POPS/ضع ألعاب PS1 المتوافقة هنا.txt",
                    "APPS/ضع تطبيقات ELF هنا.txt"}
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
