# PCSX2 runtime smoke test

This checklist is deliberately separate from host fixtures and cross-compilation.
A successful decoder test or PS2SDK build does not prove that a game boots.

## Automated artifact checks

After downloading the launcher and Neutrino artifacts from the same successful
GitHub Actions run:

1. Keep `TEa.G3.ELF` and the complete `neutrino/` directory together.
2. Verify that `TEa.G3.ELF` and `neutrino/neutrino.elf` are non-empty ELF files.
3. Verify that `neutrino/modules/cdvdman_emu.irx` is present alongside the other
   modules required by the selected Neutrino config.
4. Do not mix artifacts from different workflow runs.

## PCSX2 checks (manual, requires a legally obtained BIOS)

1. Configure PCSX2 for the matching PS2 region and a legally obtained BIOS.
2. Boot `TEa.G3.ELF` from a virtual mass-storage device or a supported ELF launch method.
3. Record whether the launcher reaches its first menu, whether Arabic text renders,
   and whether controller navigation works.
4. Put one known-good, legally owned `.iso` in `DVD/`; launch it from the menu.
5. Put a matching `.zso` test image in `DVD/`; confirm it appears with the right
   title/serial and launch it from the same menu.
6. Repeat the ZSO test after a cold emulator restart. Save the PCSX2 log and
   capture the last visible stage if the launch fails.
7. For SMB, configure a fixed PS2 IPv4 address and the `[smb]` settings in
   `tea_g3/config.ini`, export `DVD/` and/or `CD/`, generate `TEAG3.GAMES` with
   `tools/smb_catalog.py`, then select SMB and test one plain ISO.
8. Test the APA/HDLoader source on an emulated or real internal HDD separately.
   Do not count USBExtreme/UL as supported until the OPL reader is integrated.

## Current scope

CI tests the bounded decoder, the launcher's ZSO file adapter, the ordinary ISO
fallback, the SMB catalog generator, and the PS2SDK build of the rebuilt
`cdvdman_emu.irx` and SMB modules. PCSX2 game launches from ISO, ZSO, SMB and
HDLoader remain manual checks. The screenshot currently supplied shows the
launcher menu but reports `0 games loaded`, so it is not evidence of a game boot.
No end-to-end game compatibility claim is made until these checks are recorded.
