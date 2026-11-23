# TEa.G3 / OPL runtime compatibility

This document separates library discovery from formats that can actually be launched. A game appearing in the TEa.G3 library is not proof that the selected runtime can decode its image.

## Verified TEa.G3 paths at this revision

| Platform / format | Current TEa.G3 path | Status |
|---|---|---|
| PS2 ISO (`.iso`) | Neutrino through the configured source driver | Existing path; game compatibility remains title/device-specific |
| PS2 ZSO (`.zso`) | OPL ZSO/LZ4 source files are vendored under `third_party/opl-zso/` | **Not wired to runtime**; do not scan or advertise as launchable |
| PS2 CSO (`.cso`) | No decoder in the current TEa.G3 launch path | Unsupported |
| PS2 CHD (`.chd`) | No decoder in the current TEa.G3 launch path | Unsupported |
| PS2 USBExtreme / UL | Not implemented in the current TEa.G3 library/launch adapter | Unsupported |
| PS2 internal HDD APA / HDL partitions | Source discovery may identify HDD/HDL paths; per-title launch path must be checked | Do not infer all OPL HDL behavior |
| PS2 SMB share | No SMB driver/config is bundled by the current TEa.G3 Neutrino build script | Not integrated |
| PS1 VCD | POPStarter path under `POPS/` | Existing TEa.G3 path; requires POPStarter-compatible VCD files |
| PS1 BIN/CUE, CHD, PBP and other containers | No generic decoder/converter in the TEa.G3 PS1 launch path | Unsupported unless separately converted to the required POPStarter format |

## Why the vendored ZSO decoder is not enough

OPL's decoder uses `ziso_init()`, `ziso_read_sector()`, `ziso_alloc()` and `read_raw_data()`. The OPL project links its ISOFS reader into its own IOP/CDVD components. TEa.G3 currently starts games through Neutrino, whose sector-read path uses its FHI/device layer. Merely compiling the OPL C files into the EE launcher would not connect them to the disc sector requests.

Neutrino's current external core does not decode ZSO. A correct implementation must either:
1. add ZSO decoding to the Neutrino sector/FHI path with validated headers, bounded reads and compressed/raw block tests; or
2. integrate and launch an OPL-compatible native CDVD backend for sources that can supply raw sector reads.

The two approaches are not interchangeable. Do not route a ZSO game to Neutrino and claim success without a decoder in that path.

## SMB boundary

Neutrino has an SMB file-system backend composed of a network stack and SMB IOP modules, with a matching `bsd-smb.toml` configuration. TEa.G3 must build and package the matching modules/configuration, populate the network/share arguments, and select `-bsd=smb` for SMB games. The current TEa.G3 build script is pinned to an older Neutrino base and does not package these SMB components.

SMB and UDPBD/UDPFS share the PS2 network adapter. The launcher must enforce a single network owner and switch/restart the driver stack safely rather than loading both simultaneously.

## OPL features not yet ported

The OPL source tree includes separate subsystems for VMC, PS2RD cheats, per-game configuration, controller emulation, GSM/video settings, native device backends, SMB, HDD/APA and BDM. The TEa.G3 UI and Neutrino launch path do not automatically inherit these features. Each subsystem needs a compatibility review, an implementation in the appropriate EE/IOP layer, a build, and runtime tests.

Video-mode parity is intentionally outside the current integration scope.

## Validation gates

A feature may be marked supported only after all of the following:
- source and format detection tests;
- malformed/truncated/out-of-range image tests where applicable;
- PS2SDK build of the exact runtime modules;
- correct module/config packaging in the release ZIP;
- launch and gameplay verification on a real PS2, including read stability and return-to-launcher behavior.

PCSX2 and CI builds are useful checks but do not replace real-hardware validation.
