#!/usr/bin/env bash
# Build only the SMB IOP support modules needed by the TEa.G3 catalog.
# This does NOT build or package the Neutrino EE game runtime.
set -euo pipefail
export PS2DEV=/usr/local/ps2dev
export PS2SDK=/usr/local/ps2dev/ps2sdk
export GSKIT=/usr/local/ps2dev/gsKit
export PATH="$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin"

REPO="$(cd "$(dirname "$0")/.." && pwd)"
WORK="${WORK:-$HOME/tea-g3-smb-build}"
BASE=229cf1dc7a8e0e58c2da390c1caa63e2b61795dd
mkdir -p "$WORK"
cd "$WORK"

if [ ! -d neutrino-dev/.git ]; then
    git clone -q https://github.com/ps2max32/neutrino.git neutrino-dev
fi
cd neutrino-dev
git remote set-url origin https://github.com/ps2max32/neutrino.git
git fetch -q origin "$BASE"
git checkout -q -- .
git checkout -q "$BASE"
git clean -qfd iop/smap iop/ministack iop/smb
git submodule update --init iop/smb/usmb2

# The patched IOMAN supplies directory APIs for catalog browsing; OPL ETH handles game data.
python3 "$REPO/tools/patch_smb_ioman.py" "$PWD"
make -C iop/smap IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/smap.log" 2>&1 || { tail -30 "$WORK/smap.log"; exit 1; }
make -C iop/ministack IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/ministack.log" 2>&1 || { tail -30 "$WORK/ministack.log"; exit 1; }
make -C iop/smb SMB_IOMAN=1 IOP_SRXFIXUP_FLAGS="--rb --irx1 --allow-zero-text" > "$WORK/smb_ioman.log" 2>&1 || { tail -30 "$WORK/smb_ioman.log"; exit 1; }

OUT="$REPO/dist/network/modules"
mkdir -p "$OUT"
cp iop/smap/irx/smap.irx "$OUT/smap.irx"
cp iop/ministack/irx/ministack.irx "$OUT/ministack.irx"
cp iop/smb/irx/smb_ioman.irx "$OUT/smb_ioman.irx"
test -s "$OUT/smap.irx"
test -s "$OUT/ministack.irx"
test -s "$OUT/smb_ioman.irx"
test ! -e "$REPO/dist/neutrino/neutrino.elf"
ls -lh "$OUT"
echo "SMB catalog support modules built from Neutrino source $BASE; no Neutrino game ELF built."
