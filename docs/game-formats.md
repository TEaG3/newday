# Game image format support

This table separates upstream OPL capabilities, TEa.G3 discovery, and TEa.G3 launch adapters. An upstream OPL feature is not automatically a TEa.G3 feature. A scanner match alone is not a
launch-compatibility claim.

| Content | Extension / path | State on this branch |
|---|---|---|
| PlayStation 2 disc image | `.iso` in `DVD/` or `CD/` | Source- and preference-dependent: USB/BDM defaults to the OPL BDM adapter; SMB can use OPL ETH when configured; `motor = neutrino` selects Neutrino. HDD/HDL uses the OPL HDD adapter. Physical-PS2 validation remains pending |
| Compressed PS2 image | `.zso` in `DVD/` or `CD/` | The pinned OPL IOP ISOFS build includes `zso.c` and `lz4.c`; USB/BDM uses OPL BDM and SMB uses OPL ETH when configured. The EE-side `zso_ee.c` is for metadata scanning only. Neutrino is not a verified ZSO launch backend; physical-PS2 verification remains pending |
| USBExtreme split image | `ul.cfg` + `ul.*` parts | TEa.G3 now has a bounded 64-byte UL record parser, verifies every sequential split part, and routes entries to a patched OPL direct-boot path; parser, engine-selection and OPL build checks pass in CI, while physical-PS2 runtime tests remain pending |
| SMB PS2 image | `DVD/*.iso`, `CD/*.iso`, `.zso` on a share | TEa.G3 reads the generated `TEAG3.GAMES` index and prepares the SMB source. Automatic OPL selection uses the SMB/ETH adapter when its network configuration is valid; Neutrino hand-off is a separate selectable path. ZSO requires the OPL decoder. End-to-end SMB game launch tests remain pending |
| Internal HDD / HD Loader | APA partitions | TEa.G3 scans main APA headers, calculates the HDL start LBA, aggregates sub-partition sizes, and passes startup/LBA/size to the OPL mini-launch adapter. The engine policy selects OPL HDD for automatic or explicit OPL mode; OPL reads the existing `conf_hdd.cfg` and refuses launch without a configured partition. Physical-console validation remains pending |
| PlayStation 1 image | `.vcd` in `POPS/` | Existing POPStarter path; separate from Neutrino's PS2 CDVD path |
| Homebrew application | `.elf` in `APPS/` | Existing launcher path |
| CSO / Deflate | `.cso` | Not supported by the current LZ4-based ZSO reader on local sources |
| CHD | `.chd` | Not supported directly; needs a separate container/codec reader or an explicitly validated server-side conversion path |
| BIN/CUE multi-track | `.bin` + `.cue` | Not a direct supported PS2 launch format in the current scanner |
| ZIP / 7z archives | `.zip` / `.7z` | Not directly bootable; images must be extracted or converted first |

## Compatibility rule

A file extension is not enough to declare support. A format is considered
supported only when all of these work together:

1. The front-end discovers the file and obtains its title/serial without corrupting
   the normal ISO path.
2. The storage backend provides bounded random reads and seeks.
3. The selected runtime's CDVD path (OPL or Neutrino) converts logical disc-sector reads correctly.
4. CI checks valid, malformed, truncated and out-of-range images.
5. A game is launched from the actual TEa.G3 menu on a physical PS2 and sustains reads during gameplay.

The OPL IOP ISOFS build includes the ZSO/LZ4 decoder, and the TEa.G3 engine policy routes ZSO to OPL adapters where they are available. The EE-side `zso_ee.c` reader is used to inspect metadata while scanning; it is not the game-sector decoder. Do not select Neutrino for ZSO: this build has no verified Neutrino ZSO decoder. SMB catalogue discovery and OPL ETH hand-off are code-integrated, but each backend still requires end-to-end launch and sustained-read tests.

## SMB source requirements

- Configure a fixed IPv4 address, netmask and gateway under `[red]`; the SMB
  modules do not use the launcher's DHCP configuration.
- Configure `[smb]` with `server` (IPv4), `share`, `user` and `password`.
  The Neutrino SMB modules use TCP port 445.
- The share must contain `DVD/`, `CD/` and a generated `TEAG3.GAMES` file.
  Run `python tools/smb_catalog.py <local-share-root>` on the computer hosting
  the share. The index format is `relative-path<TAB>title<TAB>serial`.
- Paths are ASCII and images are ISO or ZSO files. The SMB source is read-only:
  TEa.G3 uses the physical memory card and does not create a VMC on the share.
- Keep the generated `neutrino/config/bsd-smb.toml` private: the SMB password is
  written there in plain text by the runtime configuration hand-off.
- SMB and UDPBD/UDPFS are mutually exclusive in one TEa.G3 session because they
  share the PS2 network adapter.

The current master contains source-specific runtime paths: automatic/explicit OPL selection routes USB and BDM to OPL BDM, SMB to OPL ETH when configured, and APA/HD Loader to OPL HDD; USBExtreme UL is OPL-only. Neutrino remains selectable for supported ISO paths and for other sources whose adapter policy permits it. ZSO is OPL-only in this build. The UL catalogue parser, split-part checks and engine-selection matrix are implemented and host-tested. Actual game boot and sustained-read verification on real PS2 hardware are still pending.

## Themes are independent of image formats

TEa.G3 already has twelve built-in themes and supports up to eight custom theme
folders. Custom themes can select a built-in base, set palette colors and use
an optional background picture. Layout positions, fonts, icons, sounds and
additional animation definitions remain controlled by the built-in theme.
See [themes.md](themes.md).


## OPL build status (integration branch)

The `opl-engine` CI job checks out Open PS2 Loader at
`b3a2199795fe89ba7e2dd7b843354472dff2c3eb`, fetches its separately pinned
`lwNBD` dependency using the upstream `download_lwNBD.sh` script, then builds
an uncompressed `opl.elf` and uploads it as `OPL.ELF`.

Important: `lwNBD` is not a Git submodule in the upstream OPL repository.
The upstream Makefile exposes a `download_lwNBD` target and makes `clean`
depend on it. Therefore enabling `submodules: recursive` on checkout alone
does not fetch this dependency; the workflow explicitly runs and verifies the
upstream downloader before `make clean`.

The successful CI run builds the pinned upstream OPL source with the TEa.G3
direct-boot patch, compiles the launcher and Neutrino, and passes the UL parser,
split-part and engine-selection host tests. The launcher passes the fixed-width
UL name, startup ID, media type and split count to OPL; OPL selects the USB
mass-storage driver and sets `GAME_FORMAT_USBLD` for UL. It also builds the
BDM, SMB/ETH and guarded APA/HD Loader mini-launch adapters. The remaining work is
runtime verification on a physical PS2: confirm the hand-off, actual game boot, sustained in-game reads and stability. An emulator may be used for diagnostics only and is not the acceptance reference. A green build is not a gameplay compatibility claim.

Do not mark UL or OPL engine selection as supported until those steps and
runtime tests pass.


## Installing the OPL engine artifact for a test

For a test build, copy the CI artifact's `OPL.ELF` to the root of the USB device
as `mass0:/OPL.ELF`. Keep `ul.cfg` and all `ul.*` split files in the same
root. TEa.G3 scans the UL catalogue on its USB source and only lists a record
when its metadata is valid and every declared part is present. Selecting that
entry invokes the OPL direct-boot arguments. This is an integration test
instruction, not a release-ready installer or a claim of verified gameplay.


### Selecting the runtime engine

In `mass0:/tea_g3/config.ini`, the `[juegos]` option `motor` accepts
`auto`, `neutrino`, or `opl`. Automatic mode selects OPL BDM for USB/BDM ISO
and ZSO, OPL ETH for configured SMB sources, and OPL HDD for APA/HD Loader;
USBExtreme UL is always OPL-only. `motor = neutrino` is valid for supported
ISO paths, but ZSO is rejected because this build has no verified Neutrino ZSO
decoder. `motor = opl` requests the corresponding OPL adapter. MMCE and
UDPBD/UDPFS retain their Neutrino path for supported ISO images.

## Full upstream OPL scope versus TEa.G3 adapters

The upstream OPL codebase advertises USB mass storage, MX4SIO, iLink, SMBv1 and ATA/IDE HDD backends, with ISO, ZSO, USBExtreme/UL, ELF homebrew and HDLoader HDD layouts according to backend/build. TEa.G3 currently has direct OPL adapters for USB/BDM ISO and ZSO, SMB/ETH ISO and ZSO, APA/HD Loader partitions, and USBExtreme UL. Automatic engine selection uses the available source-specific adapter; the Neutrino fallback is not a ZSO path. Each adapter still requires real-console launch and sustained-read verification. HDLoader is an HDD partition format, UL is a split USB layout, and ELF homebrew is an application category—not interchangeable disc-image formats. See [OPL runtime integration](opl-engine-integration.md) for the explicit adapter matrix.
