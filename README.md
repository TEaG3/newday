# TEa.G3 — a game launcher for the PlayStation 2

TEa.G3 is a launcher that runs on a real PS2. It lists your games with their covers, starts ISO/ZSO games through [Neutrino](https://github.com/rickgaiser/neutrino) by default and USBExtreme UL through OPL, and adds what a modern console menu has: covers downloaded by the
console itself, 3D save icons, virtual memory cards per game, an in-game menu, games served from a home server, and
RetroAchievements on real hardware. Arabic is the default interface language, with Spanish, English and Portuguese also available.

**تعديل وتطوير المطور العراقي: TEa.G3** · [دليل التثبيت العربي](INSTALL-AR.txt)

**Website:** https://teag3.github.io/newday/ · [OPL Integration Lab](https://teag3.github.io/newday/opl-lab.html) · [Manual](https://teag3.github.io/newday/manual.html) · screenshots and videos in `site/`
**Videos:** [trailer](https://youtu.be/w4V8aKWTCJw) · [setup guide](https://youtu.be/eqTNBGMIEXo)
**Download:** [latest TEa.G3 release](https://github.com/TEaG3/newday/releases/latest)

Previously named ORBIT and then NULD. Upgrading preserves existing data: the first boot automatically moves `orbit/` or `nuld/` to `tea_g3/`, installs `BOOT/TEa.G3.ELF`, and refreshes existing `BOOT/NULD.ELF` / `BOOT/ORBIT.ELF` entry points so old FreeMcBoot shortcuts continue to work. Existing `VMC/ORBIT.bin` and `VMC/NULD.bin` shared cards are reused; new installs use `VMC/TEa.G3.bin`.

![Home screen](site/img/home-carousel.jpg)

## Features

- **Three views** of your library (carousel, grid, list) at 60 frames per second, with filters (All, PS2, PS1, Apps)
  and an animated splash. Measured on the console: a median frame takes under 5 ms of the 16.7 ms budget.
- **Covers downloaded by the PS2**, over HTTPS from [xlenore/ps2-covers](https://github.com/xlenore/ps2-covers),
  decoded and dithered on the console, saved to the USB. No PC step.
- **3D save icons**: the newest save of each game, read from the memory card, spinning on the detail card.
- **Every source Neutrino knows**: USB, internal HDD (exFAT or HD Loader), MX4SIO, iLink, MMCE, UDPBD, and a
  **home game server** (udpfs + TEa.G3's catalog service) whose games list instantly and keep every feature.
- **Virtual memory cards**, one per game (or one shared), created on first play, on the game's own device.
- **Per-game options** (△): video mode forced to 480p or 1080i, compatibility modes, memory card. No 720p: the GS
  only scales lines by whole numbers, so a game's ~448 lines would come out squashed or cropped; 1080i fills an HDTV best.
- **CRT TVs**: the menu is 720p, which a CRT shows as a black screen. Press **L2+R2** together (it works blind) to
  switch it to 480i (576i on PAL consoles) and back; it is saved as `[video] pantalla` in `config.ini`. In 480i the
  games default to their native mode too.
- **In-game menu**: hold L1+L2+R1+R2+START+SELECT in any game to pause it and restart or power off; plus direct
  restart and power-off combos (In Game Reset). The rebuilt Neutrino fork uses Arabic right-to-left labels when the launcher language is Arabic.
- **RetroAchievements on the console**: the launcher identifies each game (rcheevos-compatible hash), shows how many
  achievements it has (○ opens the full list with your progress), and while you play an agent inside the game streams
  its memory to the [xeRAbora](https://github.com/hacan359/xerabora) client on your home network, which unlocks the
  achievements on your profile.
- **Discs, PS1 and apps**: boots the disc in the drive, PS1 games through POPStarter, and homebrew from `APPS/`.
- **Arabic by default**, with Spanish, English and Portuguese (`config.ini [ui] idioma`); the launcher shapes Arabic letters, joins contextual forms, lays out right-to-left text, and keeps Latin runs and numbers readable. The in-game pause menu and achievement-unlocked heading also have Arabic labels.
- Companion app: [ORBIT Jellyfin](https://github.com/danielnuld/orbit-jellyfin), your Jellyfin library on the PS2.

## Quick start

1. **Copy** `TEa.G3.ELF` and your `DVD/` / `CD/` game folders to the root of a FAT32 or exFAT USB drive. OPL, Neutrino and their required modules are embedded in TEa.G3; the first boot installs them automatically under `tea_g3/runtime/`. No separate engine files or folders are required.
   Put your PS2 ISOs in `DVD/` (and CD games in `CD/`).
2. **Start** `mass0:/TEa.G3.ELF` from uLaunchELF or FreeMcBoot (for example as FMCB's autoboot).
3. **Set up**: the first boot opens the setup wizard in Arabic (START reopens it): language, game sources, memory card. By default TEa.G3 probes USB, internal HDD/HDL, MX4SIO, iLink, MMCE, UDPBD and UDPFS at startup and scans detected sources automatically; no Discover/Show Games button is needed. You can narrow the source list in `tea_g3/config.ini`.
   video, network, the memory card for `BOOT/TEa.G3.ELF` and how FreeMcBoot 1.966 starts it (autoboot, a button or
   its menu; the original `FREEMCB.CNF` is kept as `.bak`). Every setting is also in `tea_g3/config.ini`, commented.
4. **Play**: pick a game, press ✕.

Optional:
- **Covers**: plug in the network cable; missing covers download at boot. If you see "No DNS", set
  `[red] dns = 1.1.1.1` in config.ini. Your own covers: `python3 tools/covers.py IN_DIR OUT_DIR` (numpy, Pillow)
  turns PNG/JPG files named by serial (`SLUS-21376.png`) into the `.c16` pair that goes in `mass0:/covers/`.
- **Back to TEa.G3 from a game**: with FMCB set to autoboot `BOOT/TEa.G3.ELF` (the wizard does it), "Restart" in
  the in-game menu brings you back.
- **MemCard PRO 2 / SD2PSX**: they switch to the game's own card when a game starts. To save there instead of the
  USB's virtual card, set the game's memory card to `fisica` (△, or `[memorycard] modo = fisica` for every game).
- **Achievements**: run the xeRAbora client (signed in to your RetroAchievements account) on a PC or server on the
  same network. Use [v0.1.0-alpha.9](https://github.com/hacan359/xerabora/releases/tag/v0.1.0-alpha.9), the version
  TEa.G3 is tested with: xeRAbora is in active development and newer releases may change the protocol.
- **Home game server**: on a Linux box, run Neutrino's `udpfs_server.py` and TEa.G3's `server/nuld_catalog.py`
  (zip, `server/`) over `<root>/DVD`; set `[juegos] origen = usb, udpfs` and `servidor = <server IP>`. Use a cable.
  Any other udpfs server (Server2PS2, ...): leave `servidor` empty and set a fixed `[red] ip`; no online covers or
  achievements then.
- **From an MMCE instead of a USB** (SD2PSX, MemCard PRO 2): copy `TEa.G3.ELF` to the card's SD and start
  `mmce0:/TEa.G3.ELF` (point FMCB's autoboot straight at it). Settings, covers and Neutrino live on the card; PS1
  games need TEa.G3 on a USB; MX4SIO can't be used alongside.
- **Status**: SELECT shows each game source (games found, or why it failed) and the network.

The full guide is in [the TEa.G3 site](https://github.com/TEaG3/newday/tree/master/site) and in the setup video.

## Requirements

- A PS2 that can run homebrew (FreeMcBoot or similar). Tested on a slim SCPH-75001.
- A USB drive (FAT32; exFAT for ISOs over 4 GB).
- Network (optional): built-in Ethernet on slim models, the network adapter on fat ones. Wired.

## البناء من المصدر بالعربية

للبناء على Linux أو WSL، جهّز PS2DEV/PS2SDK ومنافذ wolfSSL وlibjpeg وlibpng، ثم Python 3 والحزم Pillow وfonttools وmapbox-earcut.

```sh
./build.sh test
bash tools/build_neutrino.sh
# جهّز OPL.ELF وOPL-LICENSE.txt من بناء OPL المثبت في .github/workflows/build-ps2.yml
mkdir -p runtime-input
cp OPL.ELF OPL-LICENSE.txt runtime-input/
cp -R dist/neutrino runtime-input/neutrino
python3 tools/embed_runtime.py runtime-input "$PS2SDK/bin/bin2c"
./build.sh APP=launcher
```

يجب بناء OPL من النسخة المثبتة وتطبيق `tools/patch_opl_ul.py` كما يفعل CI؛ لا تستبدلها بملف OPL عشوائي. الناتج هو `TEa.G3.ELF` مع `runtime-manifest.txt`. راجع [دليل البناء العربي](INSTALL-AR.txt) للتفاصيل. نجاح الاختبارات والبناء لا يغني عن اختبار التشغيل على جهاز PS2 حقيقي.

## Building

Toolchain: [ps2dev](https://github.com/ps2dev/ps2dev) v2.0 (with the wolfSSL and libjpeg ports) on Linux or WSL. Python 3, Pillow, fonttools and mapbox-earcut are also required to generate the Arabic-aware font atlas and TEa.G3 vector logo.

```
./build.sh test                  # host selftests
bash tools/build_neutrino.sh     # complete Arabic-capable Neutrino runtime: dist/neutrino/
# Build/download the pinned OPL.ELF, then stage both engines:
mkdir -p runtime-input
cp OPL.ELF runtime-input/
cp -R dist/neutrino runtime-input/
python3 tools/embed_runtime.py runtime-input "$PS2SDK/bin/bin2c"
./build.sh APP=launcher          # TEa.G3.ELF with both engines embedded
./build.sh nuld_hash             # the game server's helper (static)
```

How it was built, phase by phase, is in `openspec/` (specs and archived changes) and `docs/` (results measured on
the console).

## Credits

The launcher settings panel and website footer prominently display “تعديل وتطوير المطور العراقي TEa.G3”.

- [Neutrino](https://github.com/rickgaiser/neutrino) by Maximus32 (AFL-3.0), which loads the games; TEa.G3 ships a
  fork with the in-game menu and the achievements agent (`third_party/neutrino-igr`, AFL-3.0).
- [Open PS2 Loader](https://github.com/ps2homebrew/Open-PS2-Loader) (AFL-3.0): the In Game Reset technique.
- [xeRAbora](https://github.com/hacan359/xerabora) (MIT client): the RetroAchievements protocol and PC client;
  [rcheevos](https://github.com/RetroAchievements/rcheevos) (MIT): the hash algorithm.
- [ps2sdk](https://github.com/ps2dev/ps2sdk), wolfSSL, libjpeg; Noto Sans Arabic UI (Google Fonts, SIL OFL); covers from
  [xlenore/ps2-covers](https://github.com/xlenore/ps2-covers); fonts Unbounded, Sora, Space Mono (SIL OFL).
- Developed with [Claude Code](https://claude.com/claude-code) (Anthropic) as the coding assistant, with the
  OpenSpec, Ponytail and CodeGraph plugins.

## License

GPL-3.0 (the launcher statically links wolfSSL, GPL-3.0+). The Neutrino fork in `third_party/neutrino-igr` is
AFL-3.0, as Neutrino.
