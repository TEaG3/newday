# TEa.G3 — PlayStation 2 Launcher

TEa.G3 is an Arabic right-to-left interface for managing PlayStation 2 and PlayStation 1 games and homebrew applications. **OPL is the only PS2 game engine** embedded in the launcher; there is no second game engine or silent fallback.

## Version

The current launcher version on this branch is `v1.3.13` ([published Arabic release](https://github.com/TEaG3/newday/releases/tag/arabic-v1.3.13)).

## v1.3.13 changes

- Downloaded homebrew applications are validated as ELF32/MIPS, including PT_LOAD ranges, EE-memory bounds, file bounds, and an executable entry point before the file is written.
- Fixed the second-stage ELF handoff: the launcher no longer clears all EE memory after SifLoadElf, which previously erased the program segments that had just been loaded.
- OPL remains the only PS2 game engine. SMB, UDPBD, UDPFS, and MMCE game-source launching remain disabled; real PS2 gameplay is not claimed.

## Runtime architecture

- The TEa.G3 interface manages indexing, covers, settings, and the user-facing catalogue.
- The embedded OPL runtime reads and launches PS2 games.
- USB ISO/ZSO and USBExtreme UL use OPL-specific paths.
- HDD/APA, HD Loader, MX4SIO, and iLink depend on their matching OPL adapters and still require validation on the corresponding hardware.
- SMB, UDPBD, UDPFS game streaming, and MMCE as a game source are disabled. These sources are not redirected to another engine.
- PS1 images use POPStarter, separately from the PS2 engine. ELF applications use the application loader.

## Video output

The launcher starts in interlaced 480i/576i according to the console region. This is the broadly compatible mode for composite AV and is also accepted by many Component and HDMI displays. Press L2+R2 to toggle between interlaced output and 720p. Use 720p only with a display and input that support it.

The software cannot send an independent signal to every connector simultaneously. Component/RGB colour mode also depends on the PS2 system's video-output setting and the cable or adapter. Games may change resolution after launch.

## Installation

1. Download `TEa.G3.ELF` from a GitHub Release.
2. Copy it to the root of a FAT32 USB drive.
3. Put DVD games in `DVD/` and CD games in `CD/`, using ISO or ZSO where the selected OPL adapter supports it.
4. Put USBExtreme UL files, `ul.cfg`, and all `ul.*` parts in the USB root.
5. Launch TEa.G3 through FreeMcBoot or a compatible ELF launcher.

OPL is embedded in `TEa.G3.ELF` and is installed into the runtime folder on first launch. Users do not need to copy or install a second game engine.

## Build and automated tests

Building requires PS2SDK. GitHub Actions builds the pinned OPL submodule and embeds that runtime into the single TEa.G3 launcher ELF. A successful CI build proves compilation and automated test results only; it does **not** prove that a game boots or that sustained reads work on a physical PS2.

## Hardware validation still required

- Boot and sustained-read testing for each storage source and ISO/ZSO path.
- HD Loader, MX4SIO, and iLink testing on their corresponding hardware.
- In-game reset (IGR) and return-to-launcher testing on a physical PS2.
- 720p compatibility testing across adapters and displays.

Do not mark a source as supported until its boot and sustained-read tests pass on the corresponding hardware.

## Licensing

The TEa.G3 launcher is covered by the repository's `LICENSE`. OPL retains its original license; see the included OPL license file.
