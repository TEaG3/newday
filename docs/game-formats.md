# Game image format support

This table separates formats that the TEa.G3 front-end can identify from formats
that have a complete and verified runtime path. A scanner match alone is not a
launch-compatibility claim.

| Content | Extension / path | State on this branch |
|---|---|---|
| PlayStation 2 disc image | `.iso` in `DVD/` or `CD/` | Existing Neutrino path; baseline regression test |
| Compressed PS2 image | `.zso` in `DVD/` or `CD/` | Bounded LZ4 decoding is integrated into the Neutrino FHI path on this branch; CI builds it, but PCSX2/PS2 game-launch verification remains pending |
| USBExtreme split image | `ul.cfg` + `ul.*` parts | Not implemented; requires a compatible reader/engine and validation of split ordering and metadata |
| SMB PS2 image | `DVD/*.iso`, `CD/*.iso`, `.zso` on a share | SMB source and Neutrino hand-off are wired. The generated `TEAG3.GAMES` index now lists ISO and ZSO. ZSO uses the common FHI decoder; end-to-end SMB game launch tests remain pending |
| Internal HDD / HD Loader | APA partitions | Existing APA/HDL scanner and Neutrino `-bsd=ata -bsdfs=hdl` launch path are retained |
| PlayStation 1 image | `.vcd` in `POPS/` | Existing POPStarter path; separate from Neutrino's PS2 CDVD path |
| Homebrew application | `.elf` in `APPS/` | Existing launcher path |
| CSO / Deflate | `.cso` | Not supported by the current LZ4-based ZSO reader on local sources |
| CHD | `.chd` | Not supported directly; needs a separate container/codec reader or an explicitly validated server-side conversion path |
| BIN/CUE multi-track | `.bin` + `.cue` | Not a direct supported PS2 launch format in the current scanner |
| ZIP / 7z archives | `.zip` / `.7z` | Not directly bootable; images must be extracted or converted first |

## Compatibility rule

A file extension is not enough to declare support. A format is considered
supported only when all of these work together:

1. The front-end discovers the file and obtains its title/serial without corrupting
   the normal ISO path.
2. The storage backend provides bounded random reads and seeks.
3. The Neutrino CDVD path converts logical disc-sector reads correctly.
4. CI checks valid, malformed, truncated and out-of-range images.
5. A game is launched from the actual TEa.G3 menu in PCSX2; a real-console
   check is still needed before a release claim.

ZSO decoding is placed at the common FHI sector layer. This is a code-level
integration point, not proof that every backend can run every ZSO. Each source
must pass its own read/seek and real-game tests. SMB now admits ISO and ZSO entries in its catalogue. Its ZSO hand-off is code-integrated
through the common FHI decoder, but still requires an end-to-end game launch test.

## SMB source requirements

- Configure a fixed IPv4 address, netmask and gateway under `[red]`; the SMB
  modules do not use the launcher's DHCP configuration.
- Configure `[smb]` with `server` (IPv4), `share`, `user` and `password`.
  The Neutrino SMB modules use TCP port 445.
- The share must contain `DVD/`, `CD/` and a generated `TEAG3.GAMES` file.
  Run `python tools/smb_catalog.py <local-share-root>` on the computer hosting
  the share. The index format is `relative-path<TAB>title<TAB>serial`.
- Paths are ASCII and images are ISO or ZSO files. Neutrino SMB is read-only:
  TEa.G3 uses the physical memory card and does not create a VMC on the share.
- Keep the generated `neutrino/config/bsd-smb.toml` private: the SMB password is
  written there in plain text by the runtime configuration hand-off.
- SMB and UDPBD/UDPFS are mutually exclusive in one TEa.G3 session because they
  share the PS2 network adapter.

The current build contains one PS2 game engine: Neutrino. There is not yet a
native OPL core behind TEa.G3, so there is no functional OPL engine selector.
USBExtreme/UL support and choosing between multiple game engines remain
unfinished requirements; do not represent them as available.

## Themes are independent of image formats

TEa.G3 already has twelve built-in themes and supports up to eight custom theme
folders. Custom themes can select a built-in base, set palette colors and use
an optional background picture. Layout positions, fonts, icons, sounds and
additional animation definitions remain controlled by the built-in theme.
See [themes.md](themes.md).
