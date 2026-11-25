#!/usr/bin/env python3
"""Build the OPL-only TEa.G3 release archive. Usage: python tools/make_release.py v1.0"""
import hashlib
import os
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if len(sys.argv) != 2:
    raise SystemExit("Usage: python tools/make_release.py <version>")
ver = sys.argv[1]
name = "TEa.G3-PS2"
out = os.path.join(ROOT, "dist", name + ".zip")

QUICKSTART = """TEa.G3 release
1. Copy TEa.G3.ELF to the USB root.
2. Put ISO/ZSO games in DVD/ or CD/. For USBExtreme keep ul.cfg and ul.* parts on USB.
3. Launch TEa.G3.ELF from uLaunchELF or FreeMcBoot.
4. OPL is the only PS2 engine and is embedded in TEa.G3.ELF. No Neutrino runtime is included.
5. SMB, UDPBD, UDPFS, MMCE and APA/HDL are disabled until their OPL adapters are implemented and tested.
6. See INSTALL-AR.txt for installation notes.
Iraqi developer: TEa.G3
https://github.com/TEaG3/newday
"""

os.makedirs(os.path.dirname(out), exist_ok=True)
manifest = os.path.join(ROOT, "runtime-manifest.txt")
if not os.path.isfile(manifest) or not os.path.getsize(manifest):
    raise SystemExit("runtime-manifest.txt is missing; build the OPL-only launcher first")
with open(manifest, encoding="utf-8") as f:
    embedded = f.read()
if "OPL.ELF" not in embedded or "neutrino" in embedded.lower():
    raise SystemExit("runtime manifest must contain OPL.ELF only (no Neutrino payload)")
opl_license = os.path.join(ROOT, "OPL-LICENSE.txt")
if not os.path.isfile(opl_license) or not os.path.getsize(opl_license):
    raise SystemExit("OPL-LICENSE.txt is required")

with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    def add(src, arc, mode=0o644):
        info = zipfile.ZipInfo.from_file(src, f"{name}/{arc}")
        info.external_attr = (mode | 0o100000) << 16
        with open(src, "rb") as f:
            z.writestr(info, f.read(), zipfile.ZIP_DEFLATED)

    def text(arc, value):
        info = zipfile.ZipInfo(f"{name}/{arc}", (2026, 11, 25, 0, 0, 0))
        info.external_attr = (0o100644) << 16
        z.writestr(info, value.replace("\n", "\r\n"), zipfile.ZIP_DEFLATED)

    add(os.path.join(ROOT, "TEa.G3.ELF"), "TEa.G3.ELF")
    add(manifest, "runtime-manifest.txt")
    add(opl_license, "licenses/OPL-AFL-3.0.txt")
    add(os.path.join(ROOT, "INSTALL-AR.txt"), "INSTALL-AR.txt")
    add(os.path.join(ROOT, "LICENSE"), "LICENSE")
    add(os.path.join(ROOT, "tools", "covers.py"), "tools/covers.py", 0o755)
    text("DVD/put your PS2 DVD ISOs here.txt", "")
    text("CD/put your PS2 CD ISOs here.txt", "")
    text("QUICKSTART.txt", QUICKSTART)

with open(out, "rb") as f:
    print(out, os.path.getsize(out), "bytes")
    print("zip SHA-256", hashlib.sha256(f.read()).hexdigest().upper())
