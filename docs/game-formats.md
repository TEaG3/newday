# Game image format support

This table separates formats that the TEa.G3 front-end can identify from formats
that have a complete and verified runtime path. A scanner match alone is not a
launch-compatibility claim.

| Content | Extension / path | State on this branch |
|---|---|---|
| PlayStation 2 disc image | `.iso` in `DVD/` or `CD/` | Existing Neutrino path; baseline regression test |
| Compressed PS2 image | `.zso` in `DVD/` or `CD/` | Bounded LZ4 decoding is integrated into the Neutrino FHI path on this branch; CI builds it, but PCSX2/PS2 game-launch verification remains pending |
| USBExtreme split image | `ul.cfg` + `ul.*` parts | TEa.G3 now has a bounded 64-byte UL record parser, verifies every sequential split part, and routes entries to a patched OPL direct-boot path; parser, engine-selection and OPL build checks pass in CI, while PCSX2/PS2 runtime tests remain pending |
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

The integration branch now contains two runtime paths: Neutrino remains the
default for ISO/ZSO and the existing storage sources, while a separately built
and patched OPL ELF handles USB ISO/ZSO when manually selected and USBExtreme
UL automatically. The UL catalogue parser, split-part checks and engine
selection matrix are implemented and host-tested. Actual game boot and
in-game-read verification in PCSX2 and on real PS2 hardware are still pending.

## Themes are independent of image formats

TEa.G3 already has twelve built-in themes and supports up to eight custom theme
folders. Custom themes can select a built-in base, set palette colors and use
an optional background picture. Layout positions, fonts, icons, sounds and
additional animation definitions remain controlled by the built-in theme.
See [themes.md](themes.md).


## OPL build status (integration branch)

The `opl-engine` CI job checks out Open PS2 Loader at
`b3a2199795fe89ba7e2dd7b843354472dff2c3eb`, fetches its separately pinned
`lwNBD` dependency using the upstream `download_lwNBD.sh` script, then builds
an uncompressed `opl.elf` and uploads it as `OPL.ELF`.

Important: `lwNBD` is not a Git submodule in the upstream OPL repository.
The upstream Makefile exposes a `download_lwNBD` target and makes `clean`
depend on it. Therefore enabling `submodules: recursive` on checkout alone
does not fetch this dependency; the workflow explicitly runs and verifies the
upstream downloader before `make clean`.

The successful CI run builds the pinned upstream OPL source with the TEa.G3
direct-boot patch, compiles the launcher and Neutrino, and passes the UL parser,
split-part and engine-selection host tests. The launcher passes the fixed-width
UL name, startup ID, media type and split count to OPL; OPL selects the USB
mass-storage driver and sets `GAME_FORMAT_USBLD` for UL. The remaining work is
runtime verification: confirm the hand-off and actual game boot in PCSX2 with a
configured mass-storage device, then verify in-game reads and stability on a
real PS2. A green build is not a gameplay compatibility claim.

Do not mark UL or OPL engine selection as supported until those steps and
runtime tests pass.


## Installing the OPL engine artifact for a test

For a test build, copy the CI artifact's `OPL.ELF` to the root of the USB device
as `mass0:/OPL.ELF`. Keep `ul.cfg` and all `ul.*` split files in the same
root. TEa.G3 scans the UL catalogue on its USB source and only lists a record
when its metadata is valid and every declared part is present. Selecting that
entry invokes the OPL direct-boot arguments. This is an integration test
instruction, not a release-ready installer or a claim of verified gameplay.


### Selecting the runtime engine

In `mass0:/tea_g3/config.ini`, the `[juegos]` option `motor` accepts
`auto`, `neutrino`, or `opl`. Automatic mode keeps ISO/ZSO on Neutrino and
routes USBExtreme UL to OPL. Setting `motor = opl` manually routes USB ISO/ZSO
games to OPL as well; other storage-source combinations are rejected until a
direct OPL hand-off exists for them. UL remains OPL-only.
