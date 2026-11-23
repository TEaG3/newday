# TEa.G3 SMB source setup

The SMB implementation on this branch uses Neutrino's SMB backend. It is a
read-only source and requires a complete Neutrino build containing:

- \`modules/smap.irx\`
- \`modules/ministack.irx\`
- \`modules/smb_ioman.irx\`
- \`modules/smb_fhi.irx\`
- \`config/bsd-smb.toml\` (TEa.G3 rewrites the module arguments before launch)

## Share contents

Make the exported share root contain at least one of these folders:

\`\`\`text
ShareRoot/
  DVD/
    Example Game.iso
  CD/
  TEAG3.GAMES
\`\`\`

The SMB IOMAN module supports reading a known file but does not implement
directory enumeration. TEa.G3 therefore reads the \`TEAG3.GAMES\` index from the
share root. On the computer hosting the share, run:

\`\`\`bash
python tools/smb_catalog.py /path/to/ShareRoot
\`\`\`

The script indexes \`.iso\` files under \`DVD/\` and \`CD/\` (including subfolders).
Each row in \`TEAG3.GAMES\` is tab-separated:

\`\`\`text
DVD/Example Game.iso<TAB>Example Game<TAB>SLUS_213.76
CD/Another Game.iso<TAB>Another Game<TAB>
\`\`\`

The serial field may be empty. Paths must be ASCII, relative to the share root,
and remain under \`DVD/\` or \`CD/\`.

## TEa.G3 configuration

In \`mass0:/tea_g3/config.ini\` (or the install's equivalent), enable SMB under
\`[juegos]\`:

\`\`\`ini
[juegos]
origen = smb
auto = no
\`\`\`

Set a fixed PS2 address in \`[red]\` and enter the share settings:

\`\`\`ini
[red]
ip = 192.168.1.10
mascara = 255.255.255.0
puerta = 192.168.1.1

[smb]
server = 192.168.1.20
share = PS2GAMES
user = guest
password =
\`\`\`

Replace these example addresses and names with values from the local network.
The Neutrino SMB driver uses TCP port 445 and does not use DHCP or a custom
port. The server must be compatible with Neutrino's SMB client. The current
handoff accepts an IPv4 address (not a DNS host name), a share name up to 63
ASCII bytes, and user/password values up to 31 ASCII bytes. Quotes, backslashes
and control characters are rejected because these values are written into
Neutrino's TOML module arguments.

Do not configure SMB together with UDPBD or UDPFS in one launcher session.
Those network sources share the PS2 adapter; when SMB is selected, TEa.G3
prioritises SMB and disables UDPBD/UDPFS for that session.

## Current limitations

- Only plain \`.iso\` images are listed from the SMB catalogue on this branch.
  SMB ZSO and USBExtreme/UL are not claimed as supported.
- The share is read-only. Use the PS2's physical memory card; VMC files cannot
  be written to the SMB share.
- SMB has no directory browser in this TEa.G3 integration; regenerate
  \`TEAG3.GAMES\` after adding, removing or renaming images.
- The password is saved in plain text in \`config.ini\` and in
  \`neutrino/config/bsd-smb.toml\`. Keep these files private.
- Host tests and successful compilation do not prove that a game boots. PCSX2
  testing and a real-console test are still required.
