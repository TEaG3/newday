# OPL runtime integration

## Architecture and compatibility boundary

TEa.G3 remains the front end. OPL is the automatic default for every ISO/ZSO source with an implemented OPL direct-boot adapter. Neutrino remains the automatic fallback for supported ISO paths without an OPL adapter or when the OPL runtime/source configuration is unavailable. ZSO is OPL-only in this build: it is rejected whenever a ready OPL BDM/ETH path is unavailable and is never sent to Neutrino. An explicit Neutrino preference remains available for supported ISO images only. The fresh configuration defaults to `motor = opl`; `auto` remains an explicit policy option. The launcher
uses source-specific direct-boot adapters for BDM devices (USB, MX4SIO, iLink
and ATA-backed FAT/exFAT HDD), SMB/ETH, and APA/HD Loader HDD mini-launch.
MMCE and UDP-based sources remain on Neutrino until their OPL hand-offs exist. SMB discovery currently requires a fixed PS2 IPv4 address because the Neutrino SMB index reader/ministack does not provide DHCP; a `red.ip = dhcp` setting is therefore not a valid SMB-source configuration, even if a separate OPL ETH session could obtain a lease.

The engine controller keeps image format, storage backend, runtime availability and engine choice as separate inputs. It produces an internal decision reason for each launch, writes that reason only to the private launcher log, and never exposes engine selection in the normal interface. Auto selects OPL for USB/BDM ISO/ZSO, SMB/ETH ISO/ZSO when network configuration is valid, and APA/HD Loader games when the configured OPL HDD partition exists. USBExtreme UL remains OPL-only. MMCE and UDPBD/UDPFS remain on Neutrino for supported ISO images because no OPL hand-off adapter is enabled; ZSO is rejected on those sources. If the OPL runtime or required source configuration is unavailable, ISO falls back to Neutrino with an explicit private-log reason, while ZSO fails closed. Explicit OPL preference follows the same adapter checks; explicit Neutrino preference keeps supported ISO on Neutrino and rejects ZSO. The second-stage loader validates its arguments and target ELF before the destructive memory-clear boundary. If validation or `ExecPS2` returns to the launcher, `run_loader_recover()` restores the existing VBlank handler with `gfx_resume()` and reports a launch error without reallocating framebuffers or VRAM. This recovers the UI when the hand-off returns while the launcher remains resident; it does not recover from a target that overwrites launcher memory or from a game/device crash. The controller does not infer successful gameplay from a successful ELF hand-off, and physical-PS2 validation remains necessary. The current launcher-to-OPL direct-boot adapter is narrower than upstream OPL's full feature set. Upstream OPL supports USB mass storage, MX4SIO, iLink, SMBv1 and ATA/IDE HDD backends, and ISO, ZSO, USBExtreme/UL, ELF homebrew and HDLoader HDD layouts (availability depends on OPL build and backend). The launcher must not claim that every upstream OPL capability is already exposed through TEa.G3. The current adapter matrix is:

| Format | Source | Automatic engine | Manual OPL |
|---|---|---|---|
| ISO | USB | OPL BDM | OPL BDM |
| ZSO | USB | OPL BDM | OPL BDM |
| ISO/ZSO | MX4SIO | OPL BDM (`sdc`) | OPL BDM (`sdc`) |
| ISO/ZSO | iLink | OPL BDM (`sd`) | OPL BDM (`sd`) |
| ISO/ZSO | internal FAT/exFAT HDD | OPL BDM (`ata`) | OPL BDM (`ata`) |
| USBExtreme UL | USB | OPL | OPL-only |
| ISO/ZSO | SMBv1 share | OPL ETH direct launch | OPL ETH direct launch |
| ISO | MMCE, UDPBD/UDPFS | Neutrino | Neutrino fallback (OPL adapter not enabled) |
| ZSO | MMCE, UDPBD/UDPFS | Unsupported | Rejected (no OPL BDM/ETH adapter) |
| HDLoader/APA | internal HDD | OPL HDD mini-launch (existing configured OPL partition required) | OPL HDD mini-launch |
| OPL ELF apps | APPS/ | TEa.G3 app path | Not an OPL game hand-off |

The config key is `[juegos] motor` in `tea_g3/config.ini` on the launcher's home
device:
- `auto`: OPL for ISO/ZSO on USB, MX4SIO, iLink, ATA-backed FAT/exFAT HDD and configured SMB/ETH; OPL for APA/HDL when its existing configured OPL partition is available; OPL-only for UL. Neutrino is the fallback for supported ISO paths on MMCE/UDPBD/UDPFS or when an ISO OPL runtime/source configuration is unavailable. ZSO is rejected if its OPL adapter, runtime or required SMB configuration is unavailable.
- `neutrino`: keep supported ISO images on Neutrino; reject ZSO because this build has no verified Neutrino ZSO decoder. UL remains OPL-only.
- `opl`: prefer OPL for USB, MX4SIO, iLink, ATA-backed FAT/exFAT HDD, SMB ISO/ZSO and APA/HD Loader. MMCE and UDP-based sources remain on Neutrino for supported ISO images. If a ZSO cannot use a ready OPL BDM/ETH adapter, reject it rather than falling back. UL remains OPL-only.

## Independent return path

TEa.G3 installs its small boot/recovery ELF as `mcX:/BOOT/TEa.G3.ELF` on an
available memory card. The stub is a separate executable stored on the card,
not a resident copy of the launcher in EE RAM. It initializes the IOP/USB
stack, waits for the TEa.G3 USB device, and starts `mass0:/TEa.G3.ELF`.

For OPL direct-launch modes (USBExtreme UL, BDM, SMB/ETH and APA/HDL), TEa.G3
passes the installed memory-card path as an extra launch argument. The pinned
OPL build patch assigns that path to OPL's configured exit/IGR target
(`gExitPath`). OPL documents that this target must be on a memory card; its
in-game reset sequence reloads the target ELF after resetting the game
environment. This is the independent return mechanism for OPL-launched games.
Neutrino continues to use the existing `-igrexit` / `-igrmenu` path to the
same memory-card stub.

The launcher refuses OPL direct launch if it cannot confirm that the memory
card stub was installed. If installation is still running, the UI asks the
user to retry; if no card is available or writing failed, the OPL launch is
blocked rather than silently launching without a return path.

This mechanism is a user-invoked IGR return, not an automatic recovery from
every game crash or power loss. A game that does not support IGR, has IGR
disabled by compatibility settings, or hangs before processing the reset
combination may not return. A successful OPL build does not prove that the
memory-card path works on every console. Physical PS2 testing must verify the
configured card slot, the IGR key sequence, USB reinitialization, and the
return to the TEa.G3 menu. The second-stage loader's pre-handoff failure path
also remains a separate validation item; the independent IGR stub is the
post-launch return path, not proof that every failed `ExecPS2` can recover.

## OPL build and dependency pinning

- Complete source: `TEaG3/OPL`, pinned by the `engine/opl` Git submodule
  entry. The release workflow resolves that exact gitlink SHA rather than
  maintaining a second hard-coded revision.
- `download_lwNBD.sh` is fetched from
  `ps2homebrew/Open-PS2-Loader@b3a2199795fe89ba7e2dd7b843354472dff2c3eb`;
  `lwNBD` is checked out at
  `15f1c14536d662e2e00d38d0aa91ef24149e8b96`.
- The TEaG3/OPL fork does not declare `lwNBD` as a Git submodule. CI runs the
  pinned downloader and verifies the exact dependency revision before
  `make clean`.
- The fork carries its own `lang_compiler.py`, complete Arabic translation
  source and bundled Arabic font. CI verifies all 326 base strings are
  translated, checks placeholder parity, preserves the tracked language
  assets during `make clean`, generates `lng/lang_Arabic.lng`, and checks
  that the compiled pack contains Arabic text.
- Both CI and release builds use `make NOT_PACKED=1 RTL=1 PADEMU=1`.

The direct-boot patches are maintained in `tools/patch_opl_ul.py`,
`tools/patch_opl_eth.py` and `tools/patch_opl_hdl.py`, and are applied to the
pinned TEaG3/OPL source during CI. The BDM patch:
- accepts a `bdm` startup mode with an explicit OPL driver name and device unit;
- maps USB to `usb`, MX4SIO to `sdc`, iLink to `sd`, and ATA-backed HDD to `ata`;
- accepts a `ul` startup mode plus split-part count for USBExtreme;
- selects the requested BDM device rather than the first attached device;
- sets `GAME_FORMAT_USBLD` and the UL part count for UL games;
- uses a safe `"%s"` startup-ID format string.

## APA / HD Loader mini-launch

The APA scanner now retains the exact game start LBA using the same formula as
upstream OPL: the main partition's `private_5 + (0x100000 + 4096) / 512`.
It also aggregates the sizes of every APA sub-partition sharing the same
`APA_IDMAX` prefix. The resulting `start_sector` and
`total_size_in_kb` are passed to OPL's HDD mini-launch; the full size matters
because OPL uses it to configure CDVDMAN's media-LSN bounds.

OPL resolves its HDD configuration partition itself from the existing
`hdd0:__common/OPL/conf_hdd.cfg`. In TEa.G3 mini-launch mode, OPL mounts
`__common` read-only for this lookup, validates the configured partition name,
and refuses the launch if the file, setting, partition, or existing OPL folder
is missing. It does not fall back to `+OPL`, create `conf_hdd.cfg`, create an
OPL folder, or create a missing PFS partition. The configured existing OPL PFS
partition is then mounted as OPL normally expects; OPL features such as VMC or
game configuration may write data inside that existing filesystem, but the
mini-launch adapter does not create, format, resize or rewrite APA partitions.
Normal interactive OPL behavior is left unchanged.

The global default remains `motor = auto` and selects OPL for APA/HDL when the configured OPL partition and runtime are available. If either is absent, Neutrino remains the fallback. The APA/HDL adapter still
requires physical-console validation before being advertised as hardware
verified.

## USBExtreme catalogue reader

`ul.c` parses the 64-byte upstream `USBExtreme_game_entry_t` layout:
- 32-byte fixed-width game name;
- `ul.` magic;
- startup ID, part count and media;
- the required `Byte08 = 0x08` marker.

It preserves the original fixed-width name for OPL's CRC calculation while
trimming the display title. It computes the same CRC used by upstream OPL,
constructs each expected `ul.%08X.<startup>.<part>` filename, and only exposes
a game when every sequential split part exists and is non-empty. The host
fixtures include known CRC vectors, padded names, invalid metadata and missing
split-part cases.

The launcher passes the raw fixed-width title, startup ID, CD/DVD media and
part count to `mass0:/OPL.ELF`. The OPL executable must be copied to the root
of the same USB device as `OPL.ELF`; `ul.cfg` and its `ul.*` parts stay in
that device's root as well.

## Upstream OPL capability coverage and remaining adapter work

The pinned OPL source accepts both `.iso` and `.zso` names in `isValidIsoName`; its `modules/isofs/Makefile` links `zso.o` and `lz4.o` into the ISO filesystem module. CI now checks both facts so a future upstream pin cannot silently remove ZSO parsing or the decoder. TEa.G3 has explicit direct-launch adapters for BDM, SMB/ETH and APA/HD Loader, in addition to USBExtreme UL. A successful compile does not mean every backend is hardware-verified. Remaining work is physical-console validation of sustained reads, SMB read-only behavior, VMC/compatibility settings and return-to-launcher behavior. Some formats are backend-specific: UL is a USB/BDM split layout; HDLoader is an APA/PFS HDD layout; ELF files are applications rather than PS2 disc images. Neutrino paths remain intact as the explicit user choice and as the automatic fallback for sources without an OPL adapter.

A feature is complete only after discovery, argument translation, runtime startup, sustained reads, return-to-launcher behavior and physical-console acceptance are all verified.

## Validation status

The latest verified post-merge `master` baseline is
[run #1030](https://github.com/TEaG3/newday/actions/runs/45782606867),
completed 2026-11-26 at merge commit
`6514c328b4af5583fcfbf4e28e3c6b2b0053330f` (PR #97, legacy theme renamed
to Classic). All four required jobs passed: ZSO host fixtures, pinned OPL
build, OPL/Neutrino embedding into the launcher ELF, and unified bundle
assembly.

The run validated:
- 70 Python host tests, including SMB catalogue parsing/path checks,
  release/version validation, launcher hand-off, EE headroom, Neutrino
  guardrails, branding and catalog launcher wiring;
- ISO and USBExtreme UL self-tests through `make test`;
- ZSO host fixtures for raw/compressed images, headers, bounds, I/O and
  allocation failures, plus the EE scanner adapter's decode, malformed
  input and ISO-fallback paths;
- staged catalog snapshot/diff, stable-key validation, cancellation,
  concurrent-reader/apply and download/activity gates;
- the PS2 semaphore adapter host test, including four concurrent reader
  threads, `WaitSema`/release failures and lifecycle checks;
- PS2 EE compilation of `catalog_stage.c` and `catalog_guard_ps2.c`
  with `-Werror` and an 8 KiB frame-size limit. The guard is linked into
  the launcher, but live scanning and publication remain disabled;
- Arabic-first launcher language, 38 UI codepoints and 18 IGR codepoints;
  the font self-test generated 509 glyphs in a 512x384 atlas;
- Arabic OPL translation validation: 326/326 strings, zero untranslated
  entries, placeholders preserved and bundled font present; OPL built with
  `RTL=1` and `PADEMU=1`, including DS3/DS4 USB and Bluetooth modules;
- the pinned TEaG3/OPL source
  `f53e0c5fcc177dd792453060ff008506b36c0137`, with TEa.G3 UL, BDM, ETH and
  guarded APA/HDL direct-launch patches;
- the pinned `lwNBD` dependency
  `15f1c14536d662e2e00d38d0aa91ef24149e8b96`, fetched with the upstream
  downloader from fixed Open-PS2-Loader commit
  `b3a2199795fe89ba7e2dd7b843354472dff2c3eb`;
- Neutrino from
  `229cf1dc7a8e0e58c2da390c1caa63e2b61795dd`, including SMB IOMAN
  directory operations and 9,840 bytes of EE core headroom against a
  1,024-byte minimum;
- the 45-combination website OPL route matrix and the single
  `TEa.G3.ELF` with its 76-file runtime manifest.

The OPL build emitted non-fatal upstream warnings (unused conditional
symbols and `UINT64_MAX`/`UINT64_C` redefinitions); it completed without
compiler errors. These warnings remain visible in the job log and were not
suppressed.

### Verified artifacts

The unified integration bundle is
[artifact 15098428646](https://github.com/TEaG3/newday/actions/runs/45782606867/artifacts/15098428646).
I downloaded it independently: `unzip -t` passed, all 76 manifest entries
matched their recorded byte sizes and CRC-32 values, and all 84 archive
entries were present. Its SHA-256 is:

`a6486d2c4f7532255f7677fc441c4ae7430c4dd8fdfce01aadc014cf3e854ba6`.

The three standalone artifacts were also downloaded, their ZIP integrity
checks passed, and their computed SHA-256 values matched GitHub's artifact
digests:

| Artifact | ID | SHA-256 |
|---|---:|---|
| Launcher | 15099795108 | `0ef8e66ac03773b44bef5afb2670c67f88c5b3eb893abc1323a01df21aff596b` |
| OPL | 15098997028 | `da89807b2a993b37611ef00614b08c2311f723a7f47c0cbd048e55cf5f2fe9ba` |
| Neutrino | 15099615711 | `e5e15267ce0e1597eb9d35efe5a05367e089fe0d34e7b18bf5113e0dd41a2bac` |

The files inside the unified bundle are MIPS ELF executables:
- `TEa.G3.ELF`: 10,165,492 bytes, SHA-256
  `19753192eea4ca822686a19981339a89670f6e62c4d3c1dbdd7a694e4400363e`;
- `OPL.ELF`: 5,768,732 bytes, SHA-256
  `979343189823b686eaafd76f527ab94098085857398966f941b4cd7d4a3f809c`;
- `neutrino/neutrino.elf`: 146,100 bytes, SHA-256
  `6be1b16207593fde6821539f234ad6cec3cad13ef1e8002df938362d7e51ca09`.

The bundle is a CI test artifact, not a published release. The latest
published package remains prerelease `arabic-v1.3.6`; the site continues
to select the newest published package and was not pointed at a temporary
Actions artifact.

Branding checks confirm that the visible product identity and splash use
TEa.G3. PR #97 changed the built-in legacy theme label/key to Classic while
retaining the old `nuld` key as a read-only compatibility alias. Old
boot/VMC/data-directory aliases and historical server cache/tool names
remain for migration and compatibility; do not remove them without a data
migration. The site defaults to Arabic, translates page text, and applies
RTL text runs while retaining its intentionally LTR page geometry.

The live catalog monitor remains deliberately inactive. Snapshot/diff,
staged scan, host concurrency and the PS2 semaphore adapter are tested, but
source-specific candidate adapters, per-entry network-worker leases, stable
state transfer, the render-thread safe-point swap and target hot-plug tests
are still pending. `launcher.c` still has 176 direct `cv[]` reference
lines; all readers and media workers must be integrated and tested before
activation.

CI success is not proof of game compatibility. Still required:
1. Test USB ISO/ZSO/UL adapters on a physical PS2.
2. Confirm UL discovery and sustained reads across every split part.
3. Test OPL BDM, SMB/ETH and APA/HDL launch paths on hardware.
4. Verify read-only SMB discovery, sustained reads, VMC/compatibility
   settings and return behavior on hardware.
5. Test DS3/DS4 pairing and input on a real PS2; module compilation alone
   is not controller validation.
6. Verify IGR return to TEa.G3 after each supported OPL launch route.

Do not advertise untested backend adapters as ready. Neutrino remains
available for its supported ISO paths; ZSO and UL remain OPL-only.
Physical-console acceptance is still pending.
