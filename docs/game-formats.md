# Game image format support

This table separates upstream OPL capabilities, TEa.G3 discovery, and TEa.G3 launch adapters. An upstream OPL feature is not automatically a TEa.G3 feature. A scanner match alone is not a
launch-compatibility claim.

| Content | Extension / path | State on this branch |
|---|---|---|
| PlayStation 2 disc image | `.iso` in `DVD/` or `CD/` | Source- and preference-dependent: USB/BDM defaults to the OPL BDM adapter; SMB can use OPL ETH when configured; `motor = neutrino` selects Neutrino. HDD/HDL uses the OPL HDD adapter. Physical-PS2 validation remains pending |
| Compressed PS2 image | `.zso` in `DVD/` or `CD/` | The pinned OPL IOP ISOFS build includes `zso.c` and `lz4.c`; USB/BDM uses OPL BDM and SMB uses OPL ETH when configured. The EE-side `zso_ee.c` is for metadata scanning only. Neutrino is not a verified ZSO launch backend; physical-PS2 verification remains pending |
| USBExtreme split image | `ul.cfg` + `ul.*` parts | TEa.G3 now has a bounded 64-byte UL record parser, verifies every sequential split part, and routes entries to a patched OPL direct-boot path; parser, engine-selection and OPL build checks pass in CI, while physical-PS2 runtime tests remain pending |
| SMB PS2 image | `DVD/*.iso`, `CD/*.iso`, `.zso` on a share | TEa.G3 reads the generated `TEAG3.GAMES` index and prepares the SMB source. Automatic OPL selection uses the SMB/ETH adapter when its network configuration is valid. Neutrino remains a selectable path for ISO only; ZSO requires OPL and is rejected if the SMB/ETH configuration or OPL runtime is unavailable. End-to-end SMB game launch tests remain pending |
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

- Configure a fixed IPv4 address, netmask, gateway and DNS under `[red]`;
  the current OPL ETH and Neutrino SMB hand-offs do not accept `red.ip = dhcp`.
- Configure `[smb]` with `server` (IPv4), `share`, `user` and `password`.
  The Neutrino SMB fallback uses TCP port 445; OPL ETH uses its own SMB adapter.
- The share must contain `DVD/`, `CD/` and a generated `TEAG3.GAMES` file.
  Run `python tools/smb_catalog.py <local-share-root>` on the computer hosting
  the share. The index format is `relative-path<TAB>title<TAB>serial`.
- The Neutrino SMB IOMAN is patched with dopen/dclose/dread and USMB2_FEATURE_OPENDIR; the launcher still reads TEAG3.GAMES for its game catalogue on both OPL ETH and Neutrino routes.
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


## OPL build status

The complete `TEaG3/OPL` source is pinned as the `engine/opl` submodule at
`f53e0c5fcc177dd792453060ff008506b36c0137`. CI fetches the upstream
`download_lwNBD.sh` helper and `lang_compiler.py` from the fixed upstream
Open-PS2-Loader commit `b3a2199795fe89ba7e2dd7b843354472dff2c3eb`, checks out
`lwNBD` at `15f1c14536d662e2e00d38d0aa91ef24149e8b96`, and generates the
fork's missing language header/source before compiling. The OPL build uses
`RTL=1` and `PADEMU=1`; the latter builds the DS3/DS4 USB and Bluetooth
modules. The TEa.G3 UL, BDM, ETH and guarded APA/HDL direct-launch patches are
applied to that pinned source.

The current CI build and artifact hashes are recorded in
[OPL runtime integration](opl-engine-integration.md). This verifies source
integration, compilation and packaging only. It does not verify controller
pairing, a game boot, sustained reads or return-to-launcher on a physical PS2.


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
