# Game image format support

This table separates formats that the current TEa.G3 source scanner understands
from formats that still need a decoder or runtime integration.

| Content | Extension / path | State on this branch |
|---|---|---|
| PlayStation 2 disc image | `.iso` in `DVD/` or `CD/` | Existing path; keep as the baseline regression test |
| Compressed PS2 image | `.zso` in `DVD/` or `CD/` | Runtime integration added on `integration/zso-neutrino-fhi`; CI and PCSX2 launch verification are still required |
| PlayStation 1 image | `.vcd` in `POPS/` | Existing POPStarter path; separate from Neutrino's PS2 CDVD path |
| Homebrew application | `.elf` in `APPS/` | Existing launcher path |
| CSO / Deflate | `.cso` | Not supported by the current LZ4-based ZSO decoder |
| CHD | `.chd` | Not supported; needs a separate container/codec reader |
| BIN/CUE multi-track | `.bin` + `.cue` | Not a direct supported PS2 launch format in the current scanner |
| ZIP / 7z archives | `.zip` / `.7z` | Not directly bootable; images must be extracted or converted first |

## Compatibility rule

A file extension is not enough to declare support. A format is considered
supported only when all of these work together:

1. The front-end discovers the file and obtains a title/serial without corrupting
   the normal ISO path.
2. The storage backend provides bounded random reads and seeks.
3. The Neutrino CDVD path converts logical disc-sector reads correctly.
4. CI checks valid, malformed, truncated and out-of-range images.
5. A game is launched from the actual TEa.G3 menu in PCSX2; a real-console
   check is still needed before a release claim.

ZSO decoding is placed at the common FHI sector layer so it can use any FHI
backend that is otherwise enabled. This does not automatically enable a
backend: the TEa.G3 SMB selector, network settings, credentials hand-off and
directory enumeration remain separate work.

## Themes are independent of image formats

TEa.G3 already has twelve built-in themes and supports up to eight custom theme
folders. Custom themes can select a built-in base, set palette colors and use
an optional background picture. Layout positions, fonts, icons, sounds and
additional animation definitions remain controlled by the built-in theme.
See [themes.md](themes.md).
