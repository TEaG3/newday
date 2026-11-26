# Game formats and source support

OPL is the only PS2 game engine in TEa.G3. The launcher does not fall back to another runtime.

| Format | Folder / source | Routing | Validation |
|---|---|---|---|
| PS2 ISO | DVD/ or CD/ on USB | OPL BDM / USB | Boot and sustained-read test pending |
| PS2 ZSO | DVD/ or CD/ on USB | OPL BDM / USB | OPL IOP decoder is included; hardware test pending |
| USBExtreme UL | ul.cfg and ul.* parts in USB root | OPL UL | All declared split parts are checked before launch |
| HDD ISO/ZSO | FAT32 ATA disk | OPL BDM / ATA | Device adapter test pending |
| HD Loader | APA/PFS HDD partition | OPL HDD mini-launch | Partition metadata is checked before launch |
| MX4SIO / iLink | Supported BDM device | OPL BDM | Hardware test pending |
| SMB / UDPBD / UDPFS / MMCE game source | Network / memory-card media | Disabled | No complete OPL-only discovery and sector-reader path is enabled |
| PS1 VCD | POPS/ | POPStarter | Separate application path |
| ELF application | APPS/ | Application loader | Not a PS2 disc-image route |

## Read-path correctness

Scanning an image is not the same as playing it. The launcher reads enough ISO/ZSO metadata to identify the game, but OPL must independently reopen the device and perform all game-sector reads after hand-off. The device name, unit, filename, startup ID and format must match the selected adapter. A green CI build does not prove that the PS2 hardware can enumerate the device or sustain reads.

## Video connections

The front end initializes interlaced SD output: 480i for NTSC-region consoles and 576i for PAL-region consoles. This is the default for composite AV cables (yellow video, red/white audio), and is accepted by many Component and HDMI adapters. L2+R2 toggles the launcher between interlaced SD and 720p. 720p requires a compatible display/adapter and is not a safe default for composite AV. The console RGB/YPbPr choice remains a PS2 system setting. Games may switch resolution after OPL starts.

## Current limitations

SMB, UDPBD, UDPFS game streaming and MMCE game-source launching are deliberately disabled. They must not appear as working merely because OPL upstream contains a related backend: TEa.G3 needs a complete source-discovery adapter, correct launch arguments, and successful physical-console reads before enabling them.
