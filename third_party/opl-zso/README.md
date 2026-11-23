# OPL ZSO/LZ4 reader — vendored source

This directory contains the bounded ZSO sector decoder and its LZ4 format header,
copied from [TEaG3/OPL](https://github.com/TEaG3/OPL), branch `main`.

## Runtime wiring on this branch

The `integration/zso-neutrino-fhi` branch (not yet merged/released) connects the
same decoder at two separate layers:

1. `zso_ee.c` reads a ZSO image while TEa.G3 scans its ISO9660
   `SYSTEM.CNF`, allowing the launcher to obtain the game serial and title.
2. `third_party/neutrino-igr/patch_neutrino.py` copies the decoder into the
   pinned Neutrino tree and routes CDVD FHI sector reads through it. The
   FHI interface is shared by Neutrino's supported file backends, so the
   compression layer is not tied to USB or SMB.

The build script now rebuilds and packages the modified
`cdvdman_emu.irx`. The ordinary ISO path remains unchanged. A malformed
ZSO header or a short/invalid block read returns a read error rather than
silently being treated as an uncompressed ISO.

## What is tested, and what is not

CI now has three separate layers of evidence:

- Host decoder fixtures cover raw blocks, valid LZ4 blocks, malformed blocks,
  header validation, bounds, I/O errors and allocation failure.
- A host test exercises the EE file adapter, including ISO fallback and
  truncated ZSO payloads.
- The PS2SDK workflow builds the launcher and the rebuilt Neutrino
  `cdvdman_emu.irx`.

These tests do **not** prove that a ZSO game has booted in PCSX2 or on a
physical PS2. That is still a required manual smoke test before merge/release.
The SMB source is also not yet selectable from the TEa.G3 interface: adding
ZSO at the common FHI layer does not enable SMB discovery or credentials UI.

## Format scope

This reader handles `.zso` images with ZISO/ZSO headers and LZ4-compressed
blocks. It does not add CSO/Deflate, CHD, ZIP or 7z support. Those formats
require separate decoders and independent compatibility tests.

## Licensing

The ZSO integration files are from the OPL project and are covered by its
Academic Free License 3.0; retain the upstream attribution and license notice.
The LZ4 header retains Yann Collet's BSD 2-Clause notice.
