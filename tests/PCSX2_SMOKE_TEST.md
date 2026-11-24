# TEa.G3 runtime acceptance — real PlayStation 2 is the reference

This checklist is separate from host fixtures and cross-compilation. A green
CI build does not prove that a game boots or that all split files are read.

## Automated artifact checks

Download all artifacts from one successful GitHub Actions run; do not mix runs:

1. User artifact: `TEa.G3.ELF` (contains OPL, Neutrino and their runtime modules).
2. Diagnostic CI artifacts: standalone `OPL.ELF` and `neutrino/` may be downloaded separately for inspection; they are not copied to the user's USB.

Verify that `TEa.G3.ELF` and `runtime-manifest.txt` are non-empty and that the manifest lists OPL, Neutrino, `ee_core`, `raagent`, `cdvdman_emu`, UDPFS and SMB assets.

## Optional console diagnostics (not an acceptance gate)

An emulator may be used only as a secondary diagnostic harness for UI smoke checks, crash reproduction and repeatable host-side observations. Its timing, device emulation, USB stack and hardware model differ from a physical PS2. Emulator success or failure MUST NOT be used to approve or reject hardware compatibility, and no emulator result substitutes for the tests below.

## Real PS2 acceptance tests (mandatory)

Use PCSX2 2.8.2 for this harness (not the old 2.4 setup). Configure USB1 as a mass-storage device using `tools/pcsx2/PCSX2.ini.in`; rebuild `usb.img` with `tools/pcsx2/mkimg.sh` only while PCSX2 is closed. Before booting, verify that `usb.img` exists, is FAT32, and contains `TEa.G3.ELF` plus the test folders. The configuration and image checks prove that the virtual device is prepared; only the running emulator and the launcher's `mass0:` diagnostics prove that the PS2-side USB device was enumerated.

1. Put only `TEa.G3.ELF` on a clean USB (plus the test game folder). Boot it. Verify that first boot creates `tea_g3/runtime/OPL.ELF` and `tea_g3/runtime/neutrino/neutrino.elf`, with all modules/config files, then record whether the first menu appears, Arabic text renders correctly, and controller navigation works.
2. Place one known-good, legally owned ISO in `DVD/`; confirm it appears and
   launch it with `motor = auto` (Neutrino).
3. Place a matching ZSO in `DVD/`; confirm title/serial discovery and launch
   with `motor = auto`. Repeat after a cold console restart.
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

## Independent return-path checks

The OPL auto-launch adapters now receive the installed memory-card return ELF
as their IGR/Exit target. On a physical PS2, verify each supported OPL route
separately (UL, BDM USB, SMB/ETH and APA/HDL):

1. Confirm TEa.G3 installed `mcX:/BOOT/TEa.G3.ELF` on the memory card selected
   by its startup log.
2. Launch one known-good test game through the selected OPL adapter.
3. Use OPL's documented in-game reset key sequence (L1+L2+R1+R2, then
   Start+Select) and verify that the memory-card stub starts.
4. Verify the stub reinitializes USB, finds `mass0:/TEa.G3.ELF`, and returns
   to the TEa.G3 menu.
5. Repeat after a cold boot and with the other memory-card slot when available.

The IGR path is a manual return mechanism, not an automatic rescue from a
hard-frozen game. A game-specific IGR incompatibility must be recorded as
unsupported for that title. No physical-console acceptance is implied by CI.

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

The runtime tests above are still manual. Do not mark game compatibility as verified until physical-PS2 results are recorded. Emulator-only results are diagnostic notes, never acceptance evidence.

## Evidence record (complete for each tested path)

Record console model and region, OPL/Neutrino build IDs, storage adapter and device, filesystem and partition layout, exact folder/file naming, game serial and media type, selected engine, startup result, sustained reads, gameplay duration, and any error log. Keep ISO, ZSO and UL results separate. Test cold boot and repeat launch after returning to the launcher.
