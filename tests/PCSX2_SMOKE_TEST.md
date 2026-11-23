# PCSX2 and real-PS2 runtime smoke test

This checklist is separate from host fixtures and cross-compilation. A green
CI build does not prove that a game boots or that all split files are read.

## Automated artifact checks

Download all artifacts from one successful GitHub Actions run; do not mix runs:

1. User artifact: `TEa.G3.ELF` (contains OPL, Neutrino and their runtime modules).
2. Diagnostic CI artifacts: standalone `OPL.ELF` and `neutrino/` may be downloaded separately for inspection; they are not copied to the user's USB.

Verify that `TEa.G3.ELF` and `runtime-manifest.txt` are non-empty and that the manifest lists OPL, Neutrino, `ee_core`, `raagent`, `cdvdman_emu`, UDPFS and SMB assets.

## PCSX2 checks

Use a legally obtained BIOS and a configured virtual USB mass-storage device.
If the PCSX2 setup does not expose a PS2 mass-storage device to the ELF, the
launcher can still display its UI, but the game scanner will have no USB library
to enumerate; a `0 games loaded` message is not by itself evidence of a broken
launcher UI.

1. Put only `TEa.G3.ELF` on a clean USB (plus the test game folder). Boot it. Verify that first boot creates `tea_g3/runtime/OPL.ELF` and `tea_g3/runtime/neutrino/neutrino.elf`, with all modules/config files, then record whether the first menu appears, Arabic text renders correctly, and controller navigation works.
2. Place one known-good, legally owned ISO in `DVD/`; confirm it appears and
   launch it with `motor = auto` (Neutrino).
3. Place a matching ZSO in `DVD/`; confirm title/serial discovery and launch
   with `motor = auto`. Repeat after a cold emulator restart.
4. For SMB, configure fixed PS2 IPv4 and the `[smb]` settings in
   `tea_g3/config.ini`, export `DVD/` and/or `CD/`, generate
   `TEAG3.GAMES` with `tools/smb_catalog.py`, then select SMB and test one
   plain ISO and one ZSO.
5. Test APA/HDLoader on an emulated or real internal HDD separately.
6. Confirm no root-level `OPL.ELF` or `neutrino/` folder is needed; TEa.G3 must install its embedded runtime under `tea_g3/runtime/` automatically.
7. Put a valid USBExtreme `ul.cfg` and all of its declared `ul.*` split parts
   in the same USB root. Confirm TEa.G3 lists the game only when every part is
   present, with the correct title and serial.
8. Select the UL game with `motor = auto`. Verify that TEa.G3 invokes OPL
   directly, that the selected game starts, and that reads continue across all
   split parts.
9. For an ordinary USB ISO/ZSO, set `[juegos] motor = opl` in
   `tea_g3/config.ini`, restart TEa.G3, and verify the manual OPL direct-boot
   route. Restore `motor = auto` or `motor = neutrino` afterwards.

## Real PS2 checks

Repeat USB ISO, USB ZSO, UL and manual OPL tests on real hardware. Record the
console model, storage device, filesystem, game serial, engine selection and
result. Verify in-game reads and stability, not just the first screen. Also
verify that Neutrino ISO/ZSO, SMB, and APA/HDLoader paths remain intact.

## Current automated coverage

CI currently runs:
- bounded ZSO decoder/scanner fixtures;
- UL 64-byte record parsing, fixed-width name/CRC vectors and split-part checks;
- engine-selection capability-matrix tests;
- SMB catalog generator tests;
- Arabic/Portuguese localization checks;
- TEa.G3 launcher and Neutrino PS2SDK builds;
- pinned upstream OPL checkout, pinned `lwNBD` retrieval, TEa.G3 OPL patch
  application, OPL compilation and artifact upload.

The runtime tests above are still manual. Do not mark game compatibility as
verified until PCSX2 and real-PS2 results are recorded.
