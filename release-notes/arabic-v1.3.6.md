# TEa.G3 v1.3.6 — prerelease

## Package
- Single MIPS PS2 launcher ELF: `TEa.G3.ELF`
- OPL and Neutrino runtime embedded, with the required runtime modules
- Runtime manifest: 76 entries; every file size and CRC-32 verified
- In-app version and release tag both identify `v1.3.6`

## Automated validation
- GitHub Actions master build: [45619002267](https://github.com/TEaG3/newday/actions/runs/45619002267)
- Integration bundle SHA-256: `86682ba8b8653f67acc55195a6fd2f5d4ab95da17a089cc869346c7c8f96abfe`
- Launcher ELF SHA-256: `a3e76b06f69568ca32e72c3267f03498f239c77a1213c2693176672dfb7b4512`
- ISO, UL, ZSO, engine selection, VMC, update, Arabic/font, and OPL/Neutrino build checks passed

## Acceptance still pending
This is a prerelease. The build and host tests do not prove that games boot or run correctly on a PS2. PCSX2 smoke tests and physical PS2 tests for ISO, ZSO, UL, SMB, and HDD remain outstanding.
