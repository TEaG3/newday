#!/usr/bin/env python3
"""Turn the built OPL + Neutrino runtime tree into linkable PS2SDK bin2c objects."""
from __future__ import annotations
import hashlib
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
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
    "neutrino/config/bsd-smb.toml",
)

def main() -> None:
    if len(sys.argv) not in (2, 3):
        raise SystemExit("usage: embed_runtime.py RUNTIME_ROOT [BIN2C]")
    source = Path(sys.argv[1]).resolve()
    bin2c = sys.argv[2] if len(sys.argv) == 3 else "bin2c"
    missing = [p for p in REQUIRED if not (source / p).is_file() or (source / p).stat().st_size == 0]
    if missing:
        raise SystemExit("runtime bundle is incomplete: " + ", ".join(missing))
    assets = sorted(p for p in source.rglob("*") if p.is_file())
    # Include only runtime payloads, not build logs or temporary files.
    assets = [p for p in assets if p.name not in {"build.log"} and not p.name.endswith(".tmp")]
    digest = hashlib.sha256()
    for path in assets:
        rel = path.relative_to(source).as_posix()
        digest.update(rel.encode("utf-8") + b"\0")
        digest.update(path.read_bytes())
    bundle_id = digest.hexdigest()[:16]
    out = ROOT
    declarations = []
    entries = []
    for index, path in enumerate(assets):
        symbol = f"tea_runtime_{index:03d}"
        generated = out / f"runtime_asset_{index:03d}.c"
        subprocess.run([bin2c, str(path), str(generated), symbol], check=True)
        declarations.append(f"extern unsigned char {symbol}[];")
        declarations.append(f"extern unsigned int size_{symbol};")
        entries.append(f'    {{"{path.relative_to(source).as_posix()}", {symbol}, &size_{symbol}}}')
    (out / "runtime_payload.h").write_text(
        "#ifndef TEA_RUNTIME_PAYLOAD_H\n#define TEA_RUNTIME_PAYLOAD_H\n"
        "#include <stddef.h>\n"
        "typedef struct { const char *path; const unsigned char *data; const unsigned int *size; } tea_runtime_asset;\n"
        "extern const tea_runtime_asset tea_runtime_assets[];\n"
        "extern const unsigned int tea_runtime_assets_count;\n"
        "extern const char tea_runtime_bundle_id[];\n"
        "#endif\n", encoding="utf-8")
    (out / "runtime_payload.c").write_text(
        '#include "runtime_payload.h"\n' + "\n".join(declarations) + "\n"
        "const tea_runtime_asset tea_runtime_assets[] = {\n" + ",\n".join(entries) + "\n};\n"
        f"const unsigned int tea_runtime_assets_count = {len(entries)};\n"
        f'const char tea_runtime_bundle_id[] = "{bundle_id}";\n',
        encoding="utf-8")
    (out / "runtime-manifest.txt").write_text(
        f"bundle={bundle_id}\nfiles={len(assets)}\n" +
        "".join(f"{p.relative_to(source).as_posix()}\t{p.stat().st_size}\n" for p in assets),
        encoding="utf-8")
    print(f"embedded {len(assets)} runtime files ({bundle_id})")

if __name__ == "__main__":
    main()
