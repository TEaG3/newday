# Game formats and sources

TEa.G3 uses OPL as its only PS2 game engine.

| Format / source | Current routing | Notes |
|---|---|---|
| ISO on USB | OPL BDM | Direct launch path wired |
| ZSO on USB | OPL BDM | OPL ZSO reader is included; console test pending |
| USBExtreme UL | OPL BDM | Uses the ul:<name> direct-boot record |
| HDD exFAT / MX4SIO / iLink | OPL BDM | Source selector is passed; exact device mapping still needs hardware validation |
| APA / HD Loader mini | Disabled | Existing launcher-specific argument format is not accepted by the pinned OPL fork yet |
| SMB / UDPBD / UDPFS | Disabled | No matching TEa.G3-to-OPL direct-launch adapter |
| MMCE | Disabled | No verified direct-launch mapping in this integration |
| PS1 | POPStarter | Separate from the PS2 OPL engine |
| Homebrew ELF | PS2 loader | Separate from the PS2 game engine |
| Disc boot | BIOS hand-off | Separate from OPL |

Unsupported sources fail closed. Neutrino is not used as a fallback. CI success does not equal hardware validation.
