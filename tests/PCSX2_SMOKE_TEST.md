# TEa.G3 physical-console smoke test

This plan is for a real PS2. A successful CI build or PCSX2 ELF load is not gameplay acceptance.

## Package and setup

1. Install the single user-facing file `TEa.G3.ELF`. It embeds OPL and the SMB catalog IOP support modules.
2. Do not copy a Neutrino ELF or Neutrino runtime folder; neither is part of this build.
3. The diagnostic bundle may include standalone `OPL.ELF` and `network-support/` module copies. They are not separate user entry points.
4. Use a known-good PS2, a reliable USB device, and a known-good game image. Record console model, network adapter, storage device, OPL version and TEa.G3 commit.

## Test sequence

### 1. USB / BDM ISO and ZSO
- Put one known-good ISO in `DVD/`, select USB and `motor = auto`. Confirm OPL BDM receives the expected device, startup ID and game path.
- Repeat with a known-good ZSO. Confirm OPL BDM receives it and sustain reads for at least 15 minutes.
- Repeat with `motor = opl`; it must use the same OPL route.
- Remove or corrupt the embedded OPL runtime in a test build. The launcher must block the game rather than starting another engine.

### 2. USBExtreme UL
- Put `ul.cfg` and every sequential `ul.*` part in the USB root.
- Confirm TEa.G3 rejects truncated records, missing split parts and out-of-range part counts.
- Launch one known-good UL game through OPL UL and sustain reads.

### 3. SMB / OPL ETH
- Configure static PS2 IPv4, server IPv4, share, user and password in `tea_g3/config.ini`.
- Put the generated `TEAG3.GAMES` index in the share and confirm the launcher lists only indexed, validated entries.
- Launch a known-good ISO and then ZSO through OPL ETH.
- Test wrong credentials, missing share, invalid server IP, cable disconnect and server restart. Each invalid state must fail closed; no Neutrino fallback is allowed.
- Sustain reads for at least 15 minutes and confirm no adapter/network deadlock.

### 4. APA / HD Loader HDD
- Use a disk with an existing, known-good APA/HDL game and an existing OPL configuration at `hdd0:__common/OPL/conf_hdd.cfg`.
- Confirm the launcher passes the correct startup ID, exact start LBA and aggregate sub-partition size to OPL HDD.
- Confirm OPL uses the existing configured partition and does not create or format partitions.
- Confirm ZSO is rejected for this mini-launch path.

### 5. Unsupported legacy routes
- Confirm MMCE, UDPBD and UDPFS game sources are not offered as active OPL routes.
- Set legacy `motor = neutrino` in a test configuration. The launcher must reject it and must not execute a Neutrino ELF.
- Confirm no Neutrino game runtime is extracted under `tea_g3/runtime/`.

### 6. Return and settings
- Verify OPL's own IGR controls and the TEa.G3 recovery stub/return path.
- Configure video mode, compatibility and VMC inside OPL. TEa.G3's legacy values are retained for migration only and are not applied by the OPL hand-off.
- Confirm the Arabic/English launcher UI remains responsive after a failed OPL hand-off.

## Record for each test

Record the game title and serial, format, source, engine preference, OPL adapter, startup result, sustained-read duration, network/HDD details, return behavior and any error text. Do not mark an adapter as hardware-supported until the complete test passes on a physical PS2.
