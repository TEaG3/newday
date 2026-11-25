# TEa.G3 — a PlayStation 2 launcher

TEa.G3 is an Arabic-first PlayStation 2 launcher and game catalog. **OPL is the only PS2 game engine**. The launcher routes supported formats to OPL's BDM, ETH, HDD and UL adapters. There is no Neutrino game ELF and no fallback to a second game engine. Legacy `motor = neutrino` settings are rejected.

**تعديل وتطوير المطور العراقي: TEa.G3** · [دليل التثبيت العربي](INSTALL-AR.txt)

**Website:** https://teag3.github.io/newday/ · [OPL Integration Lab](https://teag3.github.io/newday/opl-lab.html) · [Manual](https://teag3.github.io/newday/manual.html)
**Videos:** [trailer](https://youtu.be/w4V8aKWTCJw) · [setup guide](https://youtu.be/eqTNBGMIEXo)
**Download:** [latest TEa.G3 release](https://github.com/TEaG3/newday/releases/latest)

Previously named ORBIT and then NULD. Upgrading preserves existing data: the first boot migrates `orbit/` or `nuld/` to `tea_g3/` and refreshes legacy FreeMcBoot entry points. The normal user-facing executable is `TEa.G3.ELF`.

![Home screen](site/img/home-carousel.jpg)

## Features

- Three library views (carousel, grid and list), game filters, covers and save-icon previews.
- Arabic right-to-left interface by default, with English as the secondary language; Arabic shaping and mixed Latin/number layout are supported.
- OPL-only PS2 launch adapters:
  - USB, MX4SIO, iLink and supported BDM storage: OPL BDM.
  - Configured SMB share: OPL ETH.
  - APA / HD Loader internal HDD ISO: OPL HDD mini-launch.
  - USBExtreme split images: OPL UL.
- ISO and ZSO on OPL BDM/ETH. APA/HDL mini-launch accepts ISO only. UL is USB-only.
- SMB catalog discovery through the indexed `TEAG3.GAMES` share file. Three SMB IOP support modules are embedded for catalog access; OPL ETH opens the actual game data.
- PS1 through the existing POPStarter path, discs through the BIOS path, and homebrew ELF files from `APPS/`.
- RetroAchievements information in the library. The Neutrino in-game achievement agent is not part of this OPL-only runtime.
- Themes, covers, game metadata and settings remain under `tea_g3/`.

## What is not active in this build

- MMCE, UDPBD and UDPFS are not OPL game adapters and are disabled as game sources.
- Neutrino is not built, embedded or launched. Its legacy engine preference is rejected.
- The old TEa.G3 video, compatibility and virtual-memory-card values remain in configuration for migration compatibility, but the OPL hand-off does not apply them. Configure video, compatibility and VMC inside OPL.
- The former Neutrino in-game menu and achievement agent are not included. Use OPL's own IGR controls.
- CI builds do not prove gameplay. Every adapter still needs physical PS2 testing.

## USB folder template and guides

The repository includes a ready-to-copy folder layout in [USB-Template/](USB-Template/), with placeholders for `DVD/`, `CD/`, `POPS/` and `APPS/`. Read the setup instructions in [Arabic](INSTALL-AR.txt) or [English](INSTALL-EN.txt).

Keep USBExtreme/UL files (`ul.cfg` and `ul.*`) in the USB root; they do not belong inside `DVD/` or `CD/`.

## Quick start

1. Copy `TEa.G3.ELF` and your `DVD/` / `CD/` folders to the root of a FAT32 or exFAT USB drive. OPL and the SMB catalog support modules are embedded and installed automatically under `tea_g3/runtime/`.
2. Start `mass0:/TEa.G3.ELF` from uLaunchELF or FreeMcBoot.
3. Complete the Arabic setup wizard. Enable only supported sources: USB, internal HDD/HDL, MX4SIO, iLink and SMB. The default game engine is OPL; `auto` is also OPL-only.
4. Select a game and press ✕.

## SMB / ETH setup

Configure a static PS2 IPv4 address in `tea_g3/config.ini` under `[red]`, then set the server, share and credentials under `[smb]`. Place the generated `TEAG3.GAMES` index in the share. OPL ETH performs the game-data reads; the embedded SMB IOP modules are used only for catalog access.

The SMB password is stored as plain text in the local configuration. Keep `tea_g3/config.ini` private. See [docs/smb.md](docs/smb.md) for the route matrix and validation boundaries.

## Supported format matrix

| Format | Source | Engine |
|---|---|---|
| ISO | USB / BDM / MX4SIO / iLink | OPL BDM |
| ISO / ZSO | Configured SMB share | OPL ETH |
| ISO | APA / HD Loader HDD | OPL HDD |
| ZSO | USB / BDM | OPL BDM |
| UL | USB with `ul.cfg` and all split parts | OPL UL |
| ISO / ZSO | MMCE / UDPBD / UDPFS | Rejected |
| ZSO | APA / HD Loader HDD | Rejected |

## Requirements

- A PS2 capable of running homebrew (FreeMcBoot or similar).
- FAT32 or exFAT USB storage. FAT32 cannot hold a single file larger than 4 GB; use exFAT or UL split files.
- Wired network and a compatible network adapter for SMB/ETH.

## Building

Toolchain: [ps2dev](https://github.com/ps2dev/ps2dev) v2.0 with wolfSSL, libjpeg and libpng ports. Python 3, Pillow, fonttools and mapbox-earcut are required for the Arabic-aware font atlas and TEa.G3 assets.

The OPL source is pinned as the `engine/opl` submodule. Build OPL from that pin, then build only the SMB catalog IOP support modules (not the Neutrino game ELF):

```sh
git clone --recurse-submodules https://github.com/TEaG3/newday.git
cd newday
./build.sh test
bash tools/build_smb_support.sh
# Place the pinned OPL.ELF and OPL-LICENSE.txt in runtime-input/.
mkdir -p runtime-input
cp OPL.ELF OPL-LICENSE.txt runtime-input/
cp -R dist/network runtime-input/network
python3 tools/embed_runtime.py runtime-input "$PS2SDK/bin/bin2c"
./build.sh APP=launcher
```

The CI and release workflows perform the pinned OPL build and apply the TEa.G3 UL, BDM, ETH and HDD patches. Do not substitute an arbitrary OPL ELF. A successful build is not a substitute for testing on a physical PS2.

## Credits

- [Open PS2 Loader](https://github.com/ps2homebrew/Open-PS2-Loader) (AFL-3.0): the sole PS2 game engine.
- [Neutrino](https://github.com/rickgaiser/neutrino) (AFL-3.0): source for the SMB IOP support modules and retained IGR UI source only; its EE game runtime is not built or packaged.
- [xeRAbora](https://github.com/hacan359/xerabora) and [rcheevos](https://github.com/RetroAchievements/rcheevos): RetroAchievements metadata/client integration.
- [ps2sdk](https://github.com/ps2dev/ps2sdk), wolfSSL, libjpeg, Noto Sans Arabic, [xlenore/ps2-covers](https://github.com/xlenore/ps2-covers), and the Unbounded, Sora and Space Mono fonts.

## License

The launcher is GPL-3.0. OPL and the SMB IOP support modules retain their upstream AFL-3.0 notices, included separately in release packages.
