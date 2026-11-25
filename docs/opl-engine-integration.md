# OPL-only runtime integration

## Architecture

TEa.G3 remains the Arabic front end and catalog manager. **OPL is the only PS2 game engine** in this build. There is no Neutrino EE game ELF, no Neutrino game-launch branch, and no automatic fallback to another engine. Legacy `motor = neutrino` settings are rejected. Both `auto` and `opl` select OPL adapters only.

The launcher embeds `OPL.ELF` and the SMB catalog IOP support modules. The SMB modules are built from the pinned Neutrino source tree, but only the standalone `smap.irx`, `ministack.irx` and `smb_ioman.irx` support modules are packaged. They provide catalog access; they do not launch games. The Neutrino EE runtime, `smb_fhi.irx`, UDPFS/UDPBD runtime modules and Neutrino game configuration are not included.

## Adapter matrix

| Format | Source | Automatic | Explicit OPL |
|---|---|---|---|
| ISO | USB / BDM / MX4SIO / iLink / supported FAT/exFAT BDM disk | OPL BDM | OPL BDM |
| ZSO | USB / BDM | OPL BDM | OPL BDM |
| ISO | Configured SMBv1 share | OPL ETH | OPL ETH |
| ZSO | Configured SMBv1 share | OPL ETH | OPL ETH |
| ISO | APA / HD Loader internal HDD | OPL HDD mini-launch | OPL HDD mini-launch |
| ZSO | APA / HD Loader | Rejected | Rejected |
| UL | USBExtreme split files on USB | OPL UL | OPL UL |
| ISO/ZSO | MMCE, UDPBD, UDPFS or unknown source | Rejected | Rejected |

The source adapter is chosen separately from the file format. If the OPL ELF is missing, the SMB network settings are invalid, or the selected source has no OPL adapter, the launcher fails closed.

## BDM and UL

The BDM hand-off passes the selected driver/device and game metadata to OPL. Supported sources include USB, MX4SIO, iLink and the configured ATA-backed BDM route. ISO and ZSO use OPL's BDM path.

The UL parser validates the fixed-width `ul.cfg` records, checks sequential split-part existence and bounds the part count before passing metadata to OPL. UL remains USB-only and is never sent to another engine.

## SMB / ETH

TEa.G3 reads the `TEAG3.GAMES` index from the share through the three embedded SMB catalog IOP modules. It then passes the PS2 IPv4 settings, server IPv4, share and credentials to OPL's ETH adapter. OPL ETH opens the game data directly.

The SMB IOMAN source patch enables `dopen`, `dclose` and `dread` with `USMB2_FEATURE_OPENDIR`; the catalog still uses the explicit index for predictable discovery. The SMB password is stored in plain text in the local launcher configuration and must not be included in public logs or screenshots.

The launcher currently requires static IPv4 settings for SMB catalog access. SMB login, game boot and sustained reads are not yet confirmed on a physical console.

## APA / HD Loader HDD

For an APA/HDL entry, the launcher reads the main APA partition header, validates the startup ID, obtains the exact start LBA and aggregates the sizes of its matching sub-partitions. It passes these values to OPL's HDD mini-launch.

OPL reads the existing `hdd_partition` setting from `hdd0:__common/OPL/conf_hdd.cfg` through a read-only config mount. The adapter refuses launch when the configured OPL partition/folder is missing. TEa.G3 does not create or format HDD partitions. ZSO is rejected on this mini-launch path.

## Return-to-launcher behavior

The OPL adapters pass the TEa.G3 recovery stub where supported and use `run_loader_recover()` for the ELF hand-off. If the loader returns before destructive memory clearing, the wrapper restores the VBlank handler with `gfx_resume()` and reports the failure. This does not recover from a game/device crash or from a target that overwrites launcher memory. Physical IGR and return-to-launcher behavior remain unverified.

## OPL-owned settings

The old TEa.G3 video, compatibility and virtual-memory-card values are retained in configuration for migration compatibility, but the OPL hand-off does not apply those legacy values. Configure video, compatibility and VMC settings in OPL itself. The launcher must not claim that these legacy controls modify OPL's per-game configuration.

## Source pin and CI

The full `TEaG3/OPL` source is pinned as the `engine/opl` submodule at `46bad5a4a469c51e45eb1ed879fe43c100b8cb7b`. The CI and release workflows use the same OPL source and build flags, apply the TEa.G3 UL, ETH, BDM and guarded HDD patches, and build the SMB IOP support modules separately without building the Neutrino game ELF.

The embedded runtime manifest must contain `OPL.ELF`, `network/modules/smap.irx`, `network/modules/ministack.irx` and `network/modules/smb_ioman.irx`, and must not contain `neutrino/neutrino.elf`. The release package includes the OPL and SMB support-module licenses separately.

CI covers host-side engine decisions, format parsers, source metadata, SMB catalog path safety, argument validation, Arabic UI assets, release packaging and ELF builds. It is not physical-console acceptance. USB/BDM, SMB/ETH, APA/HDL and UL still require PS2 hardware tests with sustained reads and representative games.
