# SMB source and OPL ETH launch

TEa.G3 uses one PS2 game engine: **OPL**. SMB is an OPL ETH game source, not a Neutrino launch path.

## Responsibilities

- TEa.G3 reads the share's `TEAG3.GAMES` index to discover game entries and metadata.
- Three embedded IOP support modules — `smap.irx`, `ministack.irx` and `smb_ioman.irx` — provide the launcher's SMB catalog access. They are built from the pinned Neutrino source tree, but they are support modules only; no Neutrino EE game ELF, `smb_fhi.irx`, or Neutrino fallback is built or packaged.
- The launcher passes the PS2 network settings, server IPv4, share, user and password to the **OPL ETH** direct-launch adapter.
- OPL ETH opens the ISO/ZSO game data. The SMB IOP modules do not load or run games.

The SMB IOMAN patch adds `dopen`, `dclose` and `dread` support through `USMB2_FEATURE_OPENDIR`. TEa.G3 still uses the indexed `TEAG3.GAMES` catalog for predictable discovery; directory enumeration is not a substitute for the index.

## Configuration

Configure the PS2 IPv4 address and network mask in `[red]`, then set the SMB server, share and credentials in `[smb]`. The current launcher catalog path requires a static IPv4 address; DHCP is not accepted for this path.

The SMB password is stored as plain text in `tea_g3/config.ini`. Keep that file private and do not publish diagnostic bundles containing it.

The share should expose the indexed game paths read-only to the PS2 where possible. The launcher validates index entries before using them; it does not scan arbitrary server paths.

## Supported route matrix

| Format | Source | Result |
|---|---|---|
| ISO | SMB share with valid network settings | OPL ETH |
| ZSO | SMB share with valid network settings | OPL ETH |
| USBExtreme UL | SMB share | Rejected; UL is USB-only |
| APA/HDL HDD | SMB share | Rejected; use the HDD adapter |
| Any format | Invalid SMB configuration | Blocked; no fallback engine |

Legacy `motor = neutrino` settings are rejected. If OPL or the SMB configuration is unavailable, TEa.G3 fails closed rather than trying another game engine.

## Network-source boundaries

SMB and the retired UDPBD/UDPFS game sources share the PS2 network adapter. UDPBD/UDPFS are disabled in this OPL-only runtime; the source selector exposes SMB instead. The separate TEa.G3 HTTP catalog and cover services are not game engines.

## Validation status

CI validates the SMB IOMAN patch, builds the three support modules, embeds them with OPL, checks the runtime manifest and tests the OPL ETH hand-off arguments. This is source/build validation only. SMB login, game boot, sustained reads, disconnect handling and return-to-launcher behavior still require a physical PS2 test.
