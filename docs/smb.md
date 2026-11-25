# SMB source setup

SMB discovery and game launch are separate parts of the integration:

- **Discovery:** Neutrino's SMB IOMAN/FHI modules read the known file
  `smb:/TEAG3.GAMES`. They do not enumerate directories, so the host-side
  `tools/smb_catalog.py` script creates the index.
- **Launch:** with valid SMB/ETH settings, `motor = auto` and `motor = opl`
  route ISO and ZSO games to OPL ETH direct launch. Neutrino remains an
  ISO-only fallback/explicit choice. ZSO is never sent to Neutrino.

This separation matters: the Neutrino SMB modules are still needed to read the
index and for the ISO fallback, even when OPL ETH is selected to launch a game.

## Required configuration

Use a fixed IPv4 address for the PS2. The Neutrino SMB index reader/ministack
does not use the launcher's DHCP setting, so `red.ip = dhcp` is not a valid
SMB-source configuration.

Example:

```ini
[red]
ip = 192.168.1.50
mask = 255.255.255.0
gateway = 192.168.1.1

[smb]
server = 192.168.1.10
share = PS2
user = ps2
password = change-this
```

The Neutrino SMB client used for index access uses TCP port 445. OPL ETH has
its own SMB launch adapter; a successful Neutrino index read does not prove
that OPL can sustain game reads from the same server.

The runtime configuration hand-off creates
`neutrino/config/bsd-smb.toml` and includes the SMB credentials. That file
contains the password in plain text: keep it private and do not commit or share
it.

## Share layout and index

The share must contain:

```text
ShareRoot/
├── DVD/
├── CD/
└── TEAG3.GAMES
```

Generate the index on the computer hosting the share:

```bash
python tools/smb_catalog.py /path/to/ShareRoot
```

The index uses three tab-separated fields:

```text
relative-path<TAB>display-title<TAB>serial
```

For example:

```text
DVD/Example Game.iso<TAB>Example Game<TAB>SLUS_213.76
CD/Another Game.zso<TAB>Another Game<TAB>
```

Only ISO and ZSO images below `DVD/` and `CD/` are indexed. Paths must be
relative, ASCII and remain inside the share; traversal, control characters and
unsupported path forms are rejected. A serial may be empty. In that case
TEa.G3 reads `SYSTEM.CNF` from the image to discover it. For ZSO this uses the
EE-side metadata reader; that reader does not provide Neutrino with a verified
ZSO game-sector decoder.

There is no live directory browser. Regenerate `TEAG3.GAMES` after adding,
removing or renaming games. The current live-monitoring/staged-indexing work is
not enabled; it remains gated on reader leases, target synchronization tests,
safe index swapping and cancellation tests.

## Engine selection

| Content | Automatic route with valid SMB settings | Explicit Neutrino |
|---|---|---|
| ISO | OPL ETH; Neutrino fallback if the OPL runtime/source setup is unavailable | Neutrino SMB |
| ZSO | OPL ETH only; fail closed if OPL ETH is unavailable | Rejected |
| USBExtreme UL | Not an SMB format; separate USB/BDM OPL route | Not applicable |
| APA / HD Loader | Not an SMB format; separate configured OPL HDD mini-launch | Not applicable |

The route matrix and host tests cover the selection policy, catalog parsing and
build integration. They do **not** prove a game boots or sustains reads on a
physical PS2. Physical-console testing is still required for SMB ISO/ZSO
launch, long reads, read-only behavior, VMC handling and return-to-launcher
behavior.

## Operational constraints

- The SMB share is read-only from the launcher. TEa.G3 uses the physical memory
  card; it does not create a VMC on the SMB share.
- SMB and UDPBD/UDPFS are mutually exclusive in one TEa.G3 session because they
  share the PS2 network adapter.
- USBExtreme/UL belongs to the USB/BDM OPL path, not the SMB catalogue.
- Internal HDD/APA and HD Loader are handled by the separate OPL HDD adapter.
  See [OPL runtime integration](opl-engine-integration.md) for its
  `conf_hdd.cfg` preflight and hardware-validation requirements.
- Do not mark an SMB, HDD, ISO, ZSO or UL route as gameplay-verified from a
  green CI build alone.
