# TEa.G3 runtime acceptance — physical PlayStation 2 is the reference

This checklist is separate from host fixtures and cross-compilation. A green
CI build does not prove that a game boots or that all split files are read.
Keep ISO, ZSO, UL, SMB and HDD results separate.

## Automated artifact checks

Download every artifact from one successful GitHub Actions run; do not mix
files from different runs.

1. User installation artifact: `TEa.G3.ELF`. It embeds OPL, Neutrino and
   their runtime modules.
2. Diagnostic artifacts: standalone `OPL.ELF` and `neutrino/` are for
   inspection only; they are not copied to the user's USB.
3. Verify each ZIP with `unzip -t`. Confirm `TEa.G3.ELF` and
   `runtime-manifest.txt` are non-empty.
4. Verify every manifest entry by path, byte size and CRC-32. The manifest
   must include OPL, Neutrino, `ee_core`, `raagent`, `cdvdman_emu`,
   UDPFS and SMB assets.

## Optional PCSX2 diagnostics (not an acceptance gate)

Use PCSX2 2.8.2 only as a secondary diagnostic harness for UI smoke checks,
crash reproduction and repeatable observations. Emulator success or failure
MUST NOT approve or reject physical-console compatibility.

For this diagnostic harness, configure USB1 as a mass-storage device using
`tools/pcsx2/PCSX2.ini.in`; rebuild `usb.img` with
`tools/pcsx2/mkimg.sh` only while PCSX2 is closed. Before starting PCSX2,
verify that `usb.img` exists, is FAT32 and contains `TEa.G3.ELF` plus the
test folders. The configuration and image checks prove that the virtual
device is prepared; only the running emulator and its `mass0:` diagnostics
prove that PCSX2 enumerated the virtual USB device. To launch the ELF diagnostic, use `tools/pcsx2/run.sh tools/pcsx2/work/usb/TEa.G3.ELF diagnostic-run1`; keyboard pad input is available through `tools/pcsx2/key.sh`. Controller navigation, USB timing and game behavior in PCSX2 remain diagnostic observations only.

## Real PS2 acceptance tests (mandatory)

Perform these tests on a physical PS2 with a known-good, legally owned test
image. Record the console model, storage adapter, filesystem, game serial,
engine preference, startup result, sustained reads and gameplay duration.

1. Put only `TEa.G3.ELF` on a clean USB, plus the test game folders. Boot it.
   Verify that first boot creates `tea_g3/runtime/OPL.ELF` and
   `tea_g3/runtime/neutrino/neutrino.elf`, with all modules/config files.
   Record first-menu startup, Arabic rendering and controller navigation.
2. Place one known-good ISO in `DVD/`. With the default `[juegos] motor = opl`,
   verify the USB source selects OPL BDM and that the game starts with
   sustained reads. Repeat with `motor = neutrino`; ISO should use the
   Neutrino path when explicitly selected.
3. Place a matching ZSO in `DVD/`. With `motor = opl`, verify the USB
   source selects OPL BDM, the title/serial is discovered and the game starts.
   Repeat after a cold restart. Set `motor = neutrino` once and verify that
   ZSO is rejected; it must never silently fall back to Neutrino.
4. For SMB, configure a fixed PS2 IPv4 address and the `[smb]` settings in
   `tea_g3/config.ini`, export `DVD/` and/or `CD/`, and generate
   `TEAG3.GAMES` with `tools/smb_catalog.py`. With valid network settings,
   verify OPL ETH for both ISO and ZSO under the default `motor = opl`; optionally repeat with `motor = auto`.
   With invalid SMB/ETH settings, ISO may use its Neutrino fallback but ZSO
   must be rejected. Explicit `motor = neutrino` is an ISO-only path.
5. Test APA/HDLoader separately using an internal HDD. Verify an ISO through
   OPL HDD mini-launch, the exact start LBA, aggregate size and the existing
   `hdd_partition` in `conf_hdd.cfg`. ZSO is unsupported on APA/HDL and
   must be rejected.
6. Confirm no root-level `OPL.ELF` or `neutrino/` folder is needed;
   TEa.G3 must install its embedded runtime under `tea_g3/runtime/`
   automatically.
7. Put a valid USBExtreme `ul.cfg` and every declared `ul.*` split part
   in the same USB root. Confirm TEa.G3 lists the game only when every part
   is present, with the correct title and serial.
8. Select the UL game with `motor = opl` (the default). Verify TEa.G3 invokes OPL
   directly, the selected game starts and reads continue across every split
   part. UL remains OPL-only regardless of engine preference.
9. For supported USB ISO/ZSO, set `[juegos] motor = opl` in
   `tea_g3/config.ini`, restart TEa.G3 and verify the OPL direct-boot route.
   Restore the default `motor = opl` afterwards. Do not use `motor = neutrino` for
   ZSO; that selection must be rejected.

## Independent OPL return-path checks

The OPL auto-launch adapters receive the installed memory-card return ELF as
their IGR/Exit target. On a physical PS2, verify each supported OPL route
separately (UL, BDM USB, SMB/ETH and APA/HDL ISO):

1. Confirm TEa.G3 installed `mcX:/BOOT/TEa.G3.ELF` on the memory card
   selected by its startup log.
2. Launch one known-good test game through the selected OPL adapter.
3. Use OPL's documented in-game reset key sequence (L1+L2+R1+R2, then
   Start+Select) and verify that the memory-card stub starts.
4. Verify the stub reinitializes USB, finds `mass0:/TEa.G3.ELF` and returns
   to the TEa.G3 menu.
5. Repeat after a cold boot and with the other memory-card slot when available.

The IGR path is a manual return mechanism, not an automatic rescue from a
hard-frozen game. Record a game-specific IGR incompatibility as unsupported
for that title. CI and emulator results do not imply physical-console
acceptance.

## Final route matrix to verify on hardware

- ISO: OPL BDM for USB/BDM, OPL ETH for configured SMB, OPL HDD for APA/HDL;
  Neutrino remains available for explicitly selected or supported fallback
  ISO paths.
- ZSO: OPL BDM/ETH only. Reject it for Neutrino preference, missing OPL
  runtime, invalid SMB/ETH configuration, APA/HDL and sources without a
  verified OPL decoder.
- USBExtreme UL: OPL-only; all split parts must be present.
- SMB and HDD: verify sustained reads and return behavior separately.
- Arabic: verify RTL layout, connected letter forms, punctuation, numbers,
  menus and in-game return screens.

## Current automated coverage

CI currently runs:
- bounded ZSO decoder/scanner fixtures;
- UL 64-byte record parsing, fixed-width name/CRC vectors and split-part checks;
- engine-selection capability-matrix tests;
- SMB catalog generator tests;
- Arabic/Portuguese localization checks;
- site OPL route simulator matrix and documentation consistency tests;
- launcher ELF hand-off return and VBlank recovery structure tests;
- TEa.G3 launcher and Neutrino PS2SDK builds;
- pinned upstream OPL checkout, pinned `lwNBD` retrieval, TEa.G3 OPL patch
  application, OPL compilation and artifact upload.

The launcher hand-off return path also has a structural host test. On a disposable diagnostic build with an intentionally invalid OPL ELF, verify that the launch error returns to the menu and the VBlank display remains responsive; do not corrupt the runtime on a user's normal USB.

The runtime tests above are still manual. Do not mark game compatibility as
verified until physical-PS2 results are recorded. Emulator-only results are
diagnostic notes, never acceptance evidence.

## Evidence record

For each tested path, record console model/region, build IDs, storage adapter
and device, filesystem and partition layout, exact folder/file naming, game
serial and media type, selected engine, startup result, sustained reads,
gameplay duration and error log. Test cold boot and repeat launch after
returning to the launcher.
