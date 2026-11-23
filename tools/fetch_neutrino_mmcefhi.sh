#!/usr/bin/env bash
# Fetch the pinned upstream Neutrino runtime archive solely for its MMCE FHI module,
# which the current PS2SDK image does not install into PS2SDK/iop/irx.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
URL="https://github.com/ps2max32/neutrino/releases/download/latest/neutrino_v1.8.0-61-g7be8de2.7z"
SHA256="fcec9c78c8cfd844a298d16341ae6885e0ae47835070ce84e374da006c024d12"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
curl --fail --location --retry 3 "$URL" -o "$WORK/neutrino.7z"
echo "$SHA256  $WORK/neutrino.7z" | sha256sum --check -
mkdir -p "$WORK/unpacked"
bsdtar -xf "$WORK/neutrino.7z" -C "$WORK/unpacked"
MODULE="$(find "$WORK/unpacked" -type f -name mmcefhi.irx -print -quit)"
test -n "$MODULE" && test -s "$MODULE"
mkdir -p "$ROOT/third_party/neutrino-igr/runtime"
cp "$MODULE" "$ROOT/third_party/neutrino-igr/runtime/mmcefhi.irx"
sha256sum "$ROOT/third_party/neutrino-igr/runtime/mmcefhi.irx"
