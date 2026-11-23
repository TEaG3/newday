# OPL runtime integration plan

## What this branch adds

- A dedicated CI job builds the upstream Open PS2 Loader executable independently of the TEa.G3 launcher.
- OPL source is pinned to `ps2homebrew/Open-PS2-Loader@b3a2199795fe89ba7e2dd7b843354472dff2c3eb`.
- The upstream `download_lwNBD.sh` script fetches the exact `lwNBD` commit
  `15f1c14536d662e2e00d38d0aa91ef24149e8b96`.
- CI checks that dependency before running `make clean`, builds with
  `NOT_PACKED=1`, and uploads `OPL.ELF`.

## Why the previous CI diagnosis needs precision

The upstream OPL repository does not declare `lwNBD` as a Git submodule.
Its Makefile defines `download_lwNBD` and makes `clean` depend on that
target. The correct fix is to run and verify the upstream downloader in the
OPL working directory before the clean/build sequence. Recursive submodule
checkout by itself is not sufficient.

## What is not integrated yet

The standalone OPL ELF is a build artifact, not yet a TEa.G3 runtime backend.
The TEa.G3 menu does not yet route a selected game to OPL, and its library does
not yet parse `ul.cfg` and the split `ul.*` parts. Neutrino remains intact and
continues to serve its existing launch paths.

## Required next implementation stages

1. Define a common engine interface and explicit capability matrix. Keep the
   source device and image format as separate dimensions.
2. Implement OPL engine launch hand-off, including a deterministic way to
   request a selected game from the TEa.G3 front-end.
3. Implement UL parsing with strict bounds checks, part-order validation,
   missing-part detection and title/serial extraction.
4. Route UL games to OPL only; route formats unsupported by OPL to Neutrino
   when that source/format combination is actually supported.
5. Add CI fixtures for valid, malformed, truncated and missing-part UL sets.
6. Verify game launch and in-game reads in PCSX2 using a mounted virtual USB
   device, then repeat on a real PS2 before release claims.

No OPL feature should be advertised as available merely because the upstream
OPL executable compiled successfully.
