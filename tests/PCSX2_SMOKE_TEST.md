# PCSX2 runtime smoke test

This checklist is deliberately separate from the host-side ZSO fixture. A successful
cross-compile or fixture run does not prove that the PS2 executable boots in PCSX2.

## Automated artifact checks

After downloading the launcher and Neutrino artifacts from the same successful
GitHub Actions run:

1. Keep `TEa.G3.ELF` and the complete `neutrino/` directory together.
2. Verify that `TEa.G3.ELF` and `neutrino/neutrino.elf` are non-empty ELF files.
3. Verify that `neutrino/modules/` contains the modules shipped by that build.
4. Do not mix artifacts from different workflow runs.

## PCSX2 checks (manual, requires a legally obtained BIOS)

1. Configure PCSX2 for the matching PS2 region and a legally obtained BIOS.
2. Boot `TEa.G3.ELF` from a virtual mass-storage device or a supported ELF launch method.
3. Record whether the launcher reaches its first menu, whether Arabic text renders,
   and whether controller navigation works.
4. Launch a known-good homebrew ELF, then a legally owned test ISO from the supported
   local source. Record the exact point of failure and attach the PCSX2 log.
5. Repeat after a cold emulator restart.

## Current scope

The vendored ZSO decoder is not connected to Neutrino's sector-read path. The CI
fixture tests its existing raw-sector path only; it is not a compressed-ZSO,
SMB, or end-to-end game-launch test. Compressed-block validation and real PCSX2
boot results must remain pending until a bounded storage adapter is implemented
and the emulator run is performed.
