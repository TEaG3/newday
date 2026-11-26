# OPL-only runtime architecture

## Ownership and launch path

TEa.G3 owns the Arabic front end, game catalogue, cover cache, configuration and pre-launch validation. OPL is the sole PS2 game engine and the sole owner of the PS2 disc/filesystem read path after hand-off. The launcher does not bundle or invoke a second PS2 runtime.

At boot, the launcher loads its own PS2SDK IOP drivers for supported local sources, scans directory entries and reads ISO/ZSO metadata. Before hand-off it stops background readers, closes its network activity, validates the selected OPL adapter and OPL ELF, then passes source-specific direct-boot arguments to OPL. If the loader returns, the VBlank recovery wrapper restores the launcher display. A successful ELF hand-off is not proof that the game booted or sustained reads.

## Enabled routing policy

| Format | Source | TEa.G3 route | State |
|---|---|---|---|
| ISO | USB mass storage | OPL BDM / USB | Integrated; hardware test pending |
| ZSO | USB mass storage | OPL BDM / USB | Decoder present in pinned OPL build; hardware test pending |
| USBExtreme UL | USB mass storage | OPL UL | Split-part validation and direct-boot adapter integrated; hardware test pending |
| ISO/ZSO | ATA FAT32 HDD | OPL BDM / ATA | Adapter path integrated; hardware test pending |
| ISO | APA / HD Loader HDD | OPL HDD mini-launch | Metadata scan and direct-launch adapter integrated; hardware test pending |
| ISO/ZSO | MX4SIO | OPL BDM / SDC | Adapter path integrated; hardware test pending |
| ISO/ZSO | iLink | OPL BDM / SD | Adapter path integrated; hardware test pending |
| PS2 images | SMB, UDPBD, UDPFS, MMCE | Disabled / fail closed | No complete OPL-only discovery + read adapter in this build |
| PS1 VCD | USB POPS | POPStarter | Separate legacy application path |
| ELF | local/HTTP app catalogue | Application loader | Not a PS2 disc engine |

The engine policy never falls back. If the OPL runtime is missing, source mapping is unknown, the format/source pair has no adapter, or the image cannot be read, TEa.G3 refuses the launch and reports the reason.

## Read-path and source boundaries

- Launcher-side BDM modules mount supported devices for scanning. They are not the game's sector reader.
- OPL must reopen the exact same physical device and file/partition after the hand-off. Device names and unit numbers are passed explicitly.
- HD Loader is a distinct APA/PFS layout, not a regular ISO directory. Its start sector, total size and disc type are validated before OPL mini-launch.
- UL is not an ISO file: every required split part is checked before the OPL UL hand-off.
- The server catalogue is retained only for downloadable applications. It is not a remote PS2 filesystem.
- SMB, UDPBD, UDPFS streaming and MMCE game launching are disabled until an OPL-only implementation provides both catalogue discovery and a verified OPL sector-read path. There is no fallback engine.

## Video output

The launcher initializes the GS in interlaced SD mode first: NTSC 480i or PAL 576i. This gives composite AV users a compatible picture from the first frame and remains a standard input for many Component and HDMI adapters. L2+R2 toggles the launcher GS mode between interlaced SD and 720p. The 720p mode requires a compatible component/HDMI chain. The PS2 RGB/YPbPr setting is configured in the console system menu, not by the game launcher. Games may change their own display mode after OPL starts; TEa.G3 cannot force all games or all physical outputs to a single simultaneous signal.

## Validation status

CI can verify source-policy tests, OPL build, embedded runtime manifest, launcher ELF and packaging integrity. It cannot prove device enumeration, sustained reads, game boot, IGR return, or output on a physical composite/component/HDMI connection. Those remain hardware acceptance tests. Do not advertise a backend as working until it passes a real-console boot and sustained-read test.
