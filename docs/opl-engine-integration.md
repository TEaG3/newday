# OPL runtime integration

## Architecture

TEa.G3 remains the front end. Neutrino remains the default engine for ordinary
ISO/ZSO games and for the existing non-USB source routes. OPL is a separate
runtime executable, selected only for formats/source combinations that have a
direct-boot adapter in this branch.

The engine selector keeps the image format and storage source as separate
inputs. Its current capability policy is:

| Format | Source | Automatic engine | Manual OPL |
|---|---|---|---|
| ISO | USB | Neutrino | OPL |
| ZSO | USB | Neutrino | OPL |
| USBExtreme UL | USB | OPL | OPL-only |
| ISO/ZSO | SMB, APA/HDL, MX4SIO, iLink, MMCE, UDPBD/UDPFS | Neutrino | Rejected until an OPL hand-off exists |

The config key is `[juegos] motor` in `tea_g3/config.ini` on the launcher's home
device:
- `auto`: Neutrino for ISO/ZSO; OPL for UL.
- `neutrino`: keep ISO/ZSO on Neutrino; UL remains OPL-only.
- `opl`: manually route USB ISO/ZSO to OPL. Unsupported source combinations
  return an explicit message instead of silently using an incompatible path.

## OPL build and dependency pinning

- Upstream source:
  `ps2homebrew/Open-PS2-Loader@b3a2199795fe89ba7e2dd7b843354472dff2c3eb`
- Upstream `lwNBD` dependency:
  `15f1c14536d662e2e00d38d0aa91ef24149e8b96`
- The OPL repository does not declare `lwNBD` as a Git submodule. Its Makefile
  provides `download_lwNBD` and makes `clean` depend on that target.
- CI explicitly runs and verifies `download_lwNBD.sh`, applies the TEa.G3
  direct-boot patch, runs `make clean` and `make NOT_PACKED=1`, then uploads
  the standalone `OPL.ELF` artifact.

The direct-boot patch is maintained in `tools/patch_opl_ul.py` and is applied
to the pinned upstream source during CI. It:
- accepts a `usb` startup mode for USB ISO/ZSO;
- accepts a `ul` startup mode plus split-part count for USBExtreme;
- chooses the USB BDM driver for these modes;
- sets `GAME_FORMAT_USBLD` and the UL part count for UL games;
- uses a safe `"%s"` startup-ID format string.

## USBExtreme catalogue reader

`ul.c` parses the 64-byte upstream `USBExtreme_game_entry_t` layout:
- 32-byte fixed-width game name;
- `ul.` magic;
- startup ID, part count and media;
- the required `Byte08 = 0x08` marker.

It preserves the original fixed-width name for OPL's CRC calculation while
trimming the display title. It computes the same CRC used by upstream OPL,
constructs each expected `ul.%08X.<startup>.<part>` filename, and only exposes
a game when every sequential split part exists and is non-empty. The host
fixtures include known CRC vectors, padded names, invalid metadata and missing
split-part cases.

The launcher passes the raw fixed-width title, startup ID, CD/DVD media and
part count to `mass0:/OPL.ELF`. The OPL executable must be copied to the root
of the same USB device as `OPL.ELF`; `ul.cfg` and its `ul.*` parts stay in
that device's root as well.

## Validation status

The successful CI run `45246675082` validated:
- ZSO host decoder/scanner fixtures;
- UL record and split-part fixtures;
- engine-selection capability matrix;
- Arabic and Portuguese localization;
- TEa.G3 launcher and Neutrino PS2SDK builds;
- pinned OPL dependency retrieval, patch application, compilation and artifact
  upload.

CI success is not proof of game compatibility. Still required:
1. Run TEa.G3 in PCSX2 with a configured virtual USB mass-storage device.
2. Confirm UL entries are discovered and the selected game is passed to OPL.
3. Confirm the game reaches gameplay and continues reading all split parts.
4. Test manual `motor = opl` for USB ISO/ZSO.
5. Repeat on real PS2 hardware and verify stability and in-game reads.

Do not merge or advertise this path as release-ready until the runtime checks
are recorded. Neutrino must remain available as the default path.
