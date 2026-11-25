# TEa.G3 — PS2 hardware acceptance checklist

This checklist is for a physical PS2. CI and the browser route simulator do not execute games.

## Record the test environment

- PS2 model and motherboard revision:
- Console region (NTSC / PAL):
- FreeMcBoot version and launch method:
- USB device, filesystem and capacity:
- Cable: composite AV / Component / HDMI adapter:
- TV or capture-device model:
- OPL commit and TEa.G3 commit:
- Game title, serial and image hash:

## Video output

1. Boot TEa.G3 with the composite AV cable connected. Confirm a stable picture at 480i (NTSC) or 576i (PAL) from the splash screen onward.
2. Repeat with Component at interlaced SD mode.
3. Repeat with the HDMI adapter at interlaced SD mode; record whether the adapter accepts 480i/576i.
4. Press L2+R2 and confirm 720p only on a display/adapter that advertises support for it.
5. Confirm the PS2 system RGB/YPbPr setting matches the cable and TV input.
6. Do not claim that every connector can output independent signals simultaneously. Record each cable/adapter test separately.

## OPL source and read-path tests

For each enabled source, record boot result, time to first game screen, sustained play duration, scene transitions and any read errors.

- USB ISO (DVD and CD):
- USB ZSO:
- USBExtreme UL with ul.cfg and all split parts:
- ATA FAT/exFAT HDD ISO/ZSO:
- APA / HD Loader ISO mini-launch:
- MX4SIO ISO/ZSO:
- iLink ISO/ZSO:

A directory listing is not a playback test. The game must boot and read sectors continuously after OPL takes control. Repeat with at least two games and several loading/scene transitions per source.

## Fail-closed tests

- Remove OPL.ELF or invalidate its runtime CRC. TEa.G3 must refuse the launch.
- Select a network source (SMB, UDPBD or UDPFS) for a PS2 image. The launcher must report unsupported and must not start a different engine.
- Select MMCE as a game source. The launcher must refuse game launching from it.
- Confirm USB UL launch is refused if any declared ul.* part is missing.
- Confirm HD Loader launch is refused if partition metadata is incomplete.

## Return path and stability

- Test OPL's own IGR/return behaviour using its documented button combination.
- Confirm the TEa.G3 return stub restores the launcher and its display after returning.
- Test cold boot, warm boot, repeated game launches and exit/re-entry.
- Record hangs, black screens, controller issues, memory-card behaviour and any loss of source mounts.

## Acceptance criteria

Do not mark a source or video adapter as supported until its tests above pass on a physical console. Keep CI status and hardware status as separate results.
