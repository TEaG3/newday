# TEa.G3 engineering and release quality gates

This document records lessons from the NULD/ORBIT to TEa.G3 transition and the current OPL-only runtime. Historical release notes and phase reports are evidence of what happened at the time; do not rewrite old builds to make them appear to have carried current fixes.

## Regression chain: v1.2.1 to v1.2.4

The historical notes record a sequence in which a fix appeared plausible but broke or failed to restore a feature: achievements caused freezes at scene loads, the next build stopped reporting unlocks, a later build identified the wrong file/argument passed by the launcher, and the in-game menu exposed a controller-read freeze. Treat this as a regression case study, not proof that the current OPL-only code has the same defect.

For every change touching launch hand-off, DMA/SIF, pad interrupts, IGR, memory cards, runtime loading or storage reads:

1. Add a focused host fixture or structural regression test for the failure mode before changing the code.
2. Couple the test to the actual contract (argument layout, address bounds, channel bit, return path), not merely to a function name or a successful compile.
3. Run the full PS2 build and preserve its logs and artifacts; do not skip or weaken a failing check to get a green workflow.
4. Keep a separate physical-console acceptance item for behavior that host CI cannot reproduce. Record console model/revision, TEa.G3 commit, OPL build ID, SMB support-module build ID, game serial, source, OPL adapter, observed result, launcher/debug log and a photo or video when a visual symptom matters.

A host fixture proves the tested logic under its fixture assumptions. It does not prove an actual game boot, sustained reads, stable IGR, controller behavior or memory-card compatibility on hardware.

## PCSX2 is diagnostic, not a performance reference

PCSX2 is useful for repeatable correctness checks, menu navigation, crash reproduction and log collection. Historical phase-0 measurements show that its modeled GS draw timing does not track the real PS2 GS:
- Never publish FPS, frame-time, no-stutter or GS-performance claims derived from PCSX2.
- Performance claims in a release note require COP0/GS timing captured on a physical PS2, with console model, video mode, commit and raw measurement log attached.
- Preserve docs/phase0-results.md and docs/image-quality.md as historical engineering evidence; do not change their measured values during a branding cleanup.

## Features whose status must stay explicit

- **Virtual memory cards:** TEa.G3's old VMC preferences are retained for migration but are not passed by the OPL hand-off. Configure VMC inside OPL and test actual game save/load on hardware.
- **RetroAchievements:** Library metadata remains visible; the former Neutrino in-game achievement agent is not part of the OPL-only runtime. Do not claim in-game unlock telemetry.
- **IGR:** The old Neutrino in-game menu is not included. Use OPL's own IGR controls and verify the TEa.G3 recovery-stub return path on hardware.
- **Video and compatibility:** The old TEa.G3 per-game values are not applied by OPL. Configure them in OPL itself.
- **Themes/image quality:** Only report measured performance for themes and modes with raw console data. 'Ran without visible stutter' is an observation, not a measured frame-rate result.

## OPL-only routing boundary

- USB/BDM, MX4SIO, iLink and supported ATA-backed BDM storage route to OPL BDM.
- Configured SMB ISO/ZSO routes to OPL ETH. The embedded SMB IOP modules are catalog support only; OPL ETH opens game data.
- APA/HD Loader ISO routes to OPL HDD mini-launch. ZSO is rejected on this path.
- USBExtreme UL routes to OPL UL and remains USB-only.
- MMCE, UDPBD and UDPFS are not active OPL game adapters and must fail closed.
- Legacy `motor = neutrino` is rejected. No Neutrino EE game ELF is built, embedded or launched.

## Compatibility and history policy

- Keep historical release notes v1.0 through v1.2.14 unchanged. They describe ORBIT/NULD-era assets and URLs as they existed then.
- Current TEa.G3 pages, manuals, download controls and current release notes must use the TEa.G3 identity and current package.
- Keep internal names, migration sources, theme keys and deployed server paths when code depends on them. A compatibility identifier such as `nuld/`, `orbit/`, `nuld_catalog.py` or the Classic theme key is not by itself a user-visible product-brand failure.
- Do not delete old user data or rewrite historical Git tags/releases as part of a rebrand.

## Current status boundary

CI validates the OPL source pin, adapter patches, OPL BDM/ETH/HDD/UL hand-offs, SMB catalog support modules, runtime manifest, ELF packaging and host-side parsers. The runtime manifest must include OPL and the three SMB catalog modules and must not include a Neutrino game ELF. There is no longer a Neutrino EE map/headroom build because that game runtime is not part of this product.

The final acceptance source remains [docs/acceptance-tea-g3.md](acceptance-tea-g3.md). A release remains prerelease until its physical-PS2 checks and evidence record are complete.
