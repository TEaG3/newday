# OPL-only engine integration

## Decision

The launcher UI remains the TEa.G3/Nuld-derived front end. OPL is the only PS2 game engine. Neutrino is not a runtime dependency and is not invoked as a fallback.

## Pinned OPL source

Repository: https://github.com/TEaG3/OPL
Build ref: e426ab1eebaf6d0a93a03e51a6a40d028f0b422a

The complete source is tracked in `TEaG3/newday` at `engine/opl` as a Git submodule pinned to this exact commit. Use `git clone --recurse-submodules` (or `git submodule update --init --recursive`) to materialize the full source tree. The launcher repository retains its own TEa.G3 UI and embeds only the OPL runtime built from this submodule.

The build enables PADEMU=1, RTL=1 and IGS=1, generates the missing language outputs on clean checkouts, and emits a standalone OPL.ELF. The launcher embeds that ELF and verifies it through the runtime manifest.

ZSO is OPL-only in this build. The launcher restores the display with gfx_resume() after a returned ELF hand-off.\n\n## Direct-launch contract

After the loader's executable path and empty argv[0], OPL receives:

- ISO/ZSO: filename, startup, CD|DVD, bdm, massN, return-ELF.
- USBExtreme UL: ul:<name>, startup, CD|DVD, bdm, massN, return-ELF.

The OPL fork reads the optional return ELF at argv[6] and applies it to the IGR exit path after loading its config. This is a software contract; the return path still requires real-console validation.

## Frontend routing

| Source / format | Current TEa.G3 route | Status |
|---|---|---|
| USB ISO | OPL BDM with explicit mass device | Wired; hardware test pending |
| USB ZSO | OPL BDM; OPL ZSO reader | Wired; hardware test pending |
| USBExtreme UL | OPL BDM with ul: entry | Wired; hardware test pending |
| HDD exFAT, MX4SIO, iLink | OPL BDM selector | Adapter path present; device mapping needs console tests |
| SMB / UDPBD / UDPFS | None | Disabled |
| MMCE | None | Disabled |
| APA / HD Loader mini | None | Disabled pending matching OPL CLI adapter |

Unsupported sources fail closed; they do not fall back to Neutrino.

## OPL features versus TEa.G3 settings

The OPL build includes pad emulation, IGR, VMC, GSM and cheat support in its own codebase. That does not mean every TEa.G3 per-game control is already mapped into OPL configuration files. VMC, video, compatibility and cheats must be translated into OPL config/cheat files before TEa.G3 controls can claim to configure those features.

## Validation boundary

CI validates host policy/tests and builds the OPL ELF. It does not validate real USB devices, MX4SIO, HDD, controllers, IGR return, game compatibility or sustained gameplay on a PS2. No hardware-support claim should be made until those tests are performed.
