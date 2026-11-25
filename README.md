# TEa.G3

TEa.G3 keeps the Nuld/Neutrino-derived launcher experience: its themes, game library, navigation, Arabic-first layout and per-game screens. OPL is the only PS2 game engine. Neutrino is not built, embedded, loaded or used to launch games.

## Runtime architecture

- TEa.G3.ELF is the front end and the only file the user launches.
- The pinned TEa.G3 fork of Open PS2 Loader is built as OPL.ELF and embedded into the launcher runtime.
- ZSO is OPL-only in this build. USB ISO/ZSO and USBExtreme UL use the OPL BDM direct-launch interface. The frontend passes the selected BDM mass device and TEa.G3 return-stub path to OPL.
- PS1 via POPStarter, homebrew ELF applications and BIOS disc hand-off remain separate from the PS2 game engine.
- SMB, UDPBD, UDPFS, MMCE and APA/HDL direct-launch routes are disabled until a tested OPL adapter exists. They are not presented as working features.

## Build and validation

The PS2 workflow builds the pinned TEaG3/OPL source with PADEMU=1, RTL=1 and IGS=1, then embeds only OPL.ELF. Host tests cover engine policy, hand-off recovery and ZSO bounds. A successful CI build is not a substitute for testing on a real PS2.

See docs/opl-engine-integration.md, docs/game-formats.md and INSTALL-AR.txt.
