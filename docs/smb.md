# TEa.G3 SMB source setup

TEa.G3 has two SMB launch paths, selected by the engine policy:

- **OPL ETH direct launch** handles ISO and ZSO when the OPL runtime and SMB network configuration are ready. This is the automatic OPL path and the explicit `motor = opl` path.
- **Neutrino SMB** remains an ISO-only path when `motor = neutrino` is selected or an ISO falls back because the OPL adapter is unavailable. ZSO is never sent to Neutrino; it fails closed unless OPL ETH is ready.

Both paths use the share's `TEAG3.GAMES` index for game discovery. The TEa.G3 Neutrino build also patches its SMB IOMAN with dopen/dclose/dread and enables USMB2_FEATURE_OPENDIR, so the runtime has directory operations. That does not replace the launcher's indexed game-list path: launcher.c reads smb:/TEAG3.GAMES for catalog discovery on both engine routes. The index is generated on the computer hosting the share.

## Share contents

Make the exported share root contain at least one of these folders:

```text
ShareRoot/
  DVD/
    Example Game.iso
    Compressed Game.zso
  CD/
  TEAG3.GAMES
```

On the computer hosting the share, run:

```bash
python tools/smb_catalog.py /path/to/ShareRoot
```

The script indexes `.iso` and `.zso` files under `DVD/` and `CD/`, including subfolders. It writes `TEAG3.GAMES` through a temporary file and atomic replacement. Each row is tab-separated:

```text
DVD/Example Game.iso<TAB>Example Game<TAB>SLUS_213.76
DVD/Compressed Game.zso<TAB>Compressed Game<TAB>SLUS_213.77
CD/Another Game.iso<TAB>Another Game<TAB>
```

The serial field may be empty; TEa.G3 then attempts to read the image's `SYSTEM.CNF` (including through the ZSO metadata reader). Paths must be ASCII, relative to the share root, remain under `DVD/` or `CD/`, and contain no traversal, drive or control characters.

Regenerate the index after adding, removing or renaming images. There is no live directory browser in this integration.

## TEa.G3 configuration

In `mass0:/tea_g3/config.ini` (or the install's equivalent), select SMB:

```ini
[juegos]
origen = smb
auto = no

[red]
ip = 192.168.1.10
mascara = 255.255.255.0
puerta = 192.168.1.1
dns = 192.168.1.1

[smb]
server = 192.168.1.20
share = PS2GAMES
user = guest
password =
```

Replace the example addresses and names with values from the local network. Use a fixed PS2 IPv4 address; `red.ip = dhcp` is not accepted by the current SMB hand-off. The server field is an IPv4 address, not a DNS host name. Share names are limited to 63 ASCII bytes and user/password values to 31 ASCII bytes. Quotes, backslashes and control characters are rejected before values are passed to the runtime.

The Neutrino SMB fallback uses TCP port 445 and its `smb_ioman.irx`, `smb_fhi.irx` and `config/bsd-smb.toml` assets. The OPL ETH direct-launch path uses OPL's own SMB/ETH adapter and does not depend on those Neutrino modules for its game-sector reads.

Do not configure SMB together with UDPBD or UDPFS in one launcher session. Those network sources share the PS2 network adapter; when SMB is selected, TEa.G3 prioritises SMB and disables UDPBD/UDPFS for that session.

## Current limits and acceptance status

- ISO: OPL ETH is the preferred adapter when ready; Neutrino remains an ISO-only fallback.
- ZSO: OPL ETH only. If OPL or the SMB network configuration is unavailable, the launch is rejected rather than silently sent to Neutrino.
- USBExtreme UL: USB-only and OPL-only; it is not an SMB share format.
- The share is read-only. Use the PS2's physical memory card; VMC files are not written to the SMB share.
- The Neutrino fallback stores its module arguments in `neutrino/config/bsd-smb.toml`; the SMB password is plain text in the launcher configuration and generated TOML. Keep both files private.
- Host tests, a successful OPL build and a valid ELF do not prove a game boots. End-to-end SMB discovery, launch, sustained reads, VMC/compatibility behavior and return-to-launcher still require a physical PS2 test. No hardware gameplay result is claimed by CI.
