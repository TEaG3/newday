# Game formats and source routing

TEa.G3 keeps its Arabic front end and uses **OPL as its only PS2 game engine**. The engine policy does not fall back to Neutrino. Legacy `motor = neutrino` values are rejected.

## Format matrix

| Format | Location | Current route |
|---|---|---|
| PS2 ISO | USB, MX4SIO, iLink, supported BDM storage | OPL BDM |
| PS2 ISO | Configured SMB share | OPL ETH |
| PS2 ISO | APA / HD Loader internal HDD | OPL HDD mini-launch |
| PS2 ISO | MMCE, UDPBD, UDPFS or unknown source | Rejected; no OPL adapter |
| PS2 ZSO | USB/BDM | OPL BDM |
| PS2 ZSO | Configured SMB share | OPL ETH |
| PS2 ZSO | APA / HD Loader, MMCE, UDPBD/UDPFS or unknown source | Rejected |
| USBExtreme UL | USB with `ul.cfg` and all declared `ul.*` parts | OPL UL |
| PS1 VCD | `POPS/` | Existing POPStarter path; separate from the PS2 engine |
| Homebrew ELF | `APPS/` | Direct application path |

The launcher selects OPL BDM for USB/BDM sources, OPL ETH for a configured SMB share, OPL HDD for APA/HD Loader ISO entries, and OPL UL for USBExtreme split images. Unsupported sources fail closed.

## ZSO

The pinned OPL IOP ISOFS build links `zso.c` and `lz4.c` into the game-sector filesystem. USB/BDM and SMB/ETH routes can therefore use OPL's decoder. The EE-side `zso_ee.c` scanner is for metadata/catalog inspection only; it is not the game-sector decoder.

ZSO is not accepted by the APA/HDL mini-launch path. If the required OPL route or SMB network configuration is unavailable, launch is blocked.

## USBExtreme UL

TEa.G3 parses the fixed-width UL records in `ul.cfg`, bounds-checks each record, verifies all sequential split parts, and passes the title, startup ID, media type and part count to the OPL UL adapter. Keep `ul.cfg` and all `ul.*` files in the USB root. UL is USB-only.

## SMB catalog support

The launcher reads `TEAG3.GAMES` from the SMB share through the embedded `smap.irx`, `ministack.irx` and `smb_ioman.irx` support modules. These modules are compiled from the pinned Neutrino source tree but are not a game engine; the package contains no Neutrino EE game ELF or Neutrino game-launch fallback. OPL ETH performs the game-data reads.

## HDD / HD Loader

For APA/HD Loader entries, TEa.G3 reads the main APA header, obtains the exact HDL start LBA, aggregates matching sub-partition sizes, and passes the startup ID, start sector and `total_size_in_kb` to OPL's HDD mini-launch. OPL reads the existing `hdd_partition` setting from `hdd0:__common/OPL/conf_hdd.cfg` through a read-only config mount. The launcher does not create, format or repartition a disk.

ZSO is rejected for this APA/HDL mini-launch route. Use an ISO entry.

## Build and verification

The complete `TEaG3/OPL` source is pinned as the `engine/opl` submodule at commit `46bad5a4a469c51e45eb1ed879fe43c100b8cb7b`. CI applies the TEa.G3 UL, ETH, BDM and guarded HDD patches, builds OPL, builds the SMB catalog IOP modules separately, and embeds OPL plus those modules into `TEa.G3.ELF`.

CI verifies parsers, route selection, argument bounds, runtime-manifest contents and ELF construction. A successful CI build does not prove gameplay. Physical PS2 tests are still required for every adapter, sustained reads, network stability, HDD mini-launch, VMC/video configuration and return-to-launcher behavior.
