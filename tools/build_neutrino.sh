#!/bin/bash
# Build the NULD fork of Neutrino (base: rickgaiser/neutrino dev 7be8de2) into dist/neutrino/.
# usage (from Windows): wsl bash /mnt/d/ps2/ps2-launcher/tools/build_neutrino.sh
#
# Rebuilt: the loader (neutrino.elf: -igr / -igroff / -igrexit / -igrmenu / -ra options), modules/ee_core.elf (In
# Game Reset, in-game menu, achievements telemetry: third_party/neutrino-igr), modules/raagent.irx (phase 16b) and
# modules/udpfs_fhi.irx (reconnects after a lost link).
# The complete upstream module set is rebuilt from the pinned source and staged with the custom IGR/RA components.
set -e
export PS2DEV=/usr/local/ps2dev PS2SDK=/usr/local/ps2dev/ps2sdk GSKIT=/usr/local/ps2dev/gsKit
export PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin
REPO="$(cd "$(dirname "$0")/.." && pwd)"
FORK="$REPO/third_party/neutrino-igr"
python3 "$REPO/tools/fetch_arabic_font.py"
python3 "$REPO/tools/generate_igr_arabic.py"
WORK="${WORK:-$HOME/orbit-build}"
BASE=229cf1dc7a8e0e58c2da390c1caa63e2b61795dd
mkdir -p "$WORK"
cd "$WORK"
if [ ! -d neutrino-dev ]; then
    git clone -q https://github.com/ps2max32/neutrino.git neutrino-dev
fi
cd neutrino-dev
git remote set-url origin https://github.com/ps2max32/neutrino.git
git fetch -q origin "$BASE"
git checkout -q -- .
git checkout -q "$BASE"
git clean -qfd ee/ee_core ee/loader common iop/raagent iop/udpfs iop/cdvdman_emu
git submodule update --init iop/smb/usmb2
python3 "$REPO/tools/patch_smb_ioman.py" "$PWD"
rm -rf "$WORK/resetspu" && cp -r "$FORK/resetspu" "$WORK/resetspu" # the IGR's SPU2 reset, embedded in ee_core
make -C "$WORK/resetspu" > "$WORK/resetspu.log" 2>&1 || { tail -20 "$WORK/resetspu.log"; exit 1; }
bin2c "$WORK/resetspu/resetspu.irx" ee/ee_core/src/resetspu_irx.c resetspu_irx
cp "$FORK/igr.c" "$FORK/menu.c" "$FORK/menu_arabic.h" "$FORK/ra.c" ee/ee_core/src/
if [ "${MENU_TEST:-0}" = 1 ]; then # PCSX2 only: the menu guesses Black's frame (no data breakpoints there)
    sed -i 's/^#define MENU_TEST 0$/#define MENU_TEST 1/' ee/ee_core/src/menu.c
fi
cp "$FORK/igr.h" "$FORK/padpatterns.h" ee/ee_core/include/
cp "$FORK/raagent/ra_snap.h" "$FORK/raagent/ra_watch.h" common/include/ # xeRAbora's protocol: loader, ee_core, agent
rm -rf iop/raagent && cp -r "$FORK/raagent" iop/raagent
python3 "$FORK/patch_neutrino.py"
make -C iop/cdvdman_emu IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/cdvdman_emu.log" 2>&1 || { tail -20 "$WORK/cdvdman_emu.log"; exit 1; }
make -C ee/ee_core clean all > "$WORK/ee_core.log" 2>&1 || { tail -20 "$WORK/ee_core.log"; exit 1; }
grep -E ' _end$|_end = ' ee/ee_core/ee_core.map | head -2 # the 64 KB region ends at 0x94000
# the packed neutrino.elf needs ps2-packer's stubs, which this toolchain cannot find; the unpacked ELF runs the same
make -C ee/loader neutrino_unpacked.elf > "$WORK/loader.log" 2>&1 || { tail -20 "$WORK/loader.log"; exit 1; }
make -C iop/raagent > "$WORK/raagent.log" 2>&1 || { tail -20 "$WORK/raagent.log"; exit 1; }
# the game stage's udpfs: it reconnects after a lost link (patch_neutrino.py step 6)
# (--allow-zero-text: its exports.tab defines _retonly before the table, at .text 0, which this ps2sdk's iopfixup
# refuses by default; the official build's older iopfixup took it as is)
make -C iop/udpfs UDPFS_FHI=1 IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" >"$WORK/udpfs.log" 2>&1 || { tail -20 "$WORK/udpfs.log"; exit 1; }

# Build the upstream SMB stack as a separate, opt-in Neutrino backend.
# Packaging these modules does not by itself enable SMB selection in TEa.G3.
make -C iop/smap IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/smap.log" 2>&1 || { tail -20 "$WORK/smap.log"; exit 1; }
make -C iop/ministack IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/ministack.log" 2>&1 || { tail -20 "$WORK/ministack.log"; exit 1; }
make -C iop/smb SMB_IOMAN=1 IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/smb_ioman.log" 2>&1 || { tail -20 "$WORK/smb_ioman.log"; exit 1; }
make -C iop/smb SMB_FHI=1 IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/smb_fhi.log" 2>&1 || { tail -20 "$WORK/smb_fhi.log"; exit 1; }

mkdir -p "$REPO/dist/neutrino/modules" "$REPO/dist/neutrino/config"
cp ee/loader/neutrino_unpacked.elf "$REPO/dist/neutrino/neutrino.elf"
cp ee/ee_core/ee_core.elf iop/raagent/irx/raagent.irx iop/udpfs/irx/udpfs_fhi.irx \
   iop/cdvdman_emu/irx/cdvdman_emu.irx "$REPO/dist/neutrino/modules/"
cp iop/smap/irx/smap.irx iop/ministack/irx/ministack.irx \
   iop/smb/irx/smb_ioman.irx iop/smb/irx/smb_fhi.irx "$REPO/dist/neutrino/modules/"
cp ee/loader/config/bsd-smb.toml "$REPO/dist/neutrino/config/"
# Build the remaining upstream IOP modules from this exact Neutrino source revision. The loader target is
# intentionally omitted: this toolchain cannot produce its packed ELF, and the custom unpacked ELF above is used.
sed -e '/$(MAKE) -C ee\/loader.*all DEBUG=0/d' Makefile > Makefile.embedded
# The PS2SDK image does not ship mmcefhi.irx. Remove its copy recipe explicitly;
# the SHA-256-pinned release module is staged below instead.
python3 - <<'PYFIX'
from pathlib import Path
path = Path("Makefile.embedded")
lines = path.read_text(encoding="utf-8").splitlines()
filtered = [line for line in lines if "mmcefhi.irx" not in line]
path.write_text("\n".join(filtered) + "\n", encoding="utf-8")
if any("mmcefhi.irx" in line for line in path.read_text(encoding="utf-8").splitlines()):
    raise SystemExit("mmcefhi.irx copy recipe remains in Makefile.embedded")
PYFIX
make -f Makefile.embedded all IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/neutrino_modules.log" 2>&1 || {
    tail -40 "$WORK/neutrino_modules.log"; exit 1;
}
make -f Makefile.embedded copy > "$WORK/neutrino_copy.log" 2>&1 || {
    tail -40 "$WORK/neutrino_copy.log"; exit 1;
}
make -f Makefile.embedded copy_extra >> "$WORK/neutrino_copy.log" 2>&1 || {
    tail -40 "$WORK/neutrino_copy.log"; exit 1;
}
mkdir -p "$REPO/dist/neutrino/modules" "$REPO/dist/neutrino/config"
cp -R ee/loader/modules/. "$REPO/dist/neutrino/modules/"
cp -R ee/loader/config/. "$REPO/dist/neutrino/config/"
# PS2SDK does not ship mmcefhi.irx; use the SHA-256-pinned official Neutrino release module.
MMCE_FHI="$REPO/third_party/neutrino-igr/runtime/mmcefhi.irx"
test -s "$MMCE_FHI"
cp "$MMCE_FHI" "$REPO/dist/neutrino/modules/mmcefhi.irx"
# Restore the TEa.G3-specific achievements agent after the upstream module copy.
cp iop/raagent/irx/raagent.irx "$REPO/dist/neutrino/modules/raagent.irx"
test -s "$REPO/dist/neutrino/modules/cdvdman_emu.irx"
test -s "$REPO/dist/neutrino/modules/ee_core.elf"
test -s "$REPO/dist/neutrino/modules/udnl.irx"
test -s "$REPO/dist/neutrino/modules/mmcefhi.irx"
ls -la "$REPO/dist/neutrino" "$REPO/dist/neutrino/modules" "$REPO/dist/neutrino/config"
echo "complete Neutrino runtime staged at dist/neutrino/ from $BASE"
