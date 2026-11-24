# OPL runtime integration

## Architecture and compatibility boundary

TEa.G3 remains the front end. Neutrino remains the default engine for ordinary
ISO/ZSO games and for the existing non-USB source routes. OPL is a separate
runtime executable, selected only for formats/source combinations that have a
direct-boot adapter in this branch.

The engine selector keeps image format, storage backend and runtime engine as separate inputs. The current launcher-to-OPL direct-boot adapter is narrower than upstream OPL's full feature set. Upstream OPL supports USB mass storage, MX4SIO, iLink, SMBv1 and ATA/IDE HDD backends, and ISO, ZSO, USBExtreme/UL, ELF homebrew and HDLoader HDD layouts (availability depends on OPL build and backend). The launcher must not claim that every upstream OPL capability is already exposed through TEa.G3. The current adapter matrix is:

| Format | Source | Automatic engine | Manual OPL |
|---|---|---|---|
| ISO | USB | Neutrino | OPL |
| ZSO | USB | Neutrino | OPL |
| USBExtreme UL | USB | OPL | OPL-only |
| ISO/ZSO | SMB, APA/HDL, MX4SIO, iLink, MMCE, UDPBD/UDPFS | Neutrino | Not yet adapted |
| HDLoader/APA | internal HDD | Neutrino | Not yet adapted |
| OPL ELF apps | APPS/ | TEa.G3 app path | Not an OPL game hand-off |

The config key is `[juegos] motor` in `tea_g3/config.ini` on the launcher's home
device:
- `auto`: Neutrino for ISO/ZSO; OPL for UL.
- `neutrino`: keep ISO/ZSO on Neutrino; UL remains OPL-only.
- `opl`: manually route USB ISO/ZSO to OPL. Unadapted source combinations must report the precise limitation; do not silently route them to an incompatible OPL mode.

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

## Upstream OPL capability coverage and remaining adapter work

The OPL build is included, but a successful compile does not mean TEa.G3 can yet invoke every OPL backend. The remaining integration work is to implement and test source-specific hand-offs for MX4SIO, iLink, SMBv1 and ATA/IDE HDD, plus the matching per-backend ISO/ZSO/UL/HDL layouts. Some formats are backend-specific: UL is a USB/BDM split layout; HDLoader is an APA/PFS HDD layout; ELF files are applications rather than PS2 disc images. Neutrino paths must remain intact while each OPL adapter is added. Do not solve a missing adapter by hiding the source or deleting the option.

A feature is complete only after discovery, argument translation, runtime startup, sustained reads, return-to-launcher behavior and physical-console acceptance are all verified.

## Validation status

The successful CI run `45337162329` validated:
- ZSO host decoder/scanner fixtures;
- UL record and split-part fixtures;
- engine-selection capability matrix;
- Arabic and Portuguese localization;
- TEa.G3 launcher and Neutrino PS2SDK builds;
- pinned OPL dependency retrieval, patch application, compilation and artifact
  upload; the final bundle job also assembled and uploaded the combined test package.

CI success is not proof of game compatibility. Still required:
1. Test the existing USB ISO/ZSO/UL adapters on a physical PS2.
2. Confirm UL entries are discovered and the selected game is passed to OPL.
3. Confirm gameplay and sustained reads across every UL split part.
4. Test manual `motor = opl` for USB ISO/ZSO.
5. Implement and test source-specific OPL adapters for MX4SIO, iLink, SMBv1 and ATA/IDE HDD without regressing Neutrino.
6. Verify each backend's supported ISO/ZSO/UL/HDL or ELF application layout on physical hardware.

The unified test bundle is available from [CI run #45247633906](https://github.com/TEaG3/newday/actions/runs/45337162329) as `tea-g3-integration-test-bundle-cbce23482d2ac918d723be0f5c9ab767afaf0c74`.

Do not advertise untested backend adapters as ready. Neutrino must remain available as the existing compatible path until the corresponding OPL adapter has passed physical-console tests.
