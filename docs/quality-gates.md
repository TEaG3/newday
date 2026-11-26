# TEa.G3 engineering and release quality gates

This document records historical lessons from the NULD/ORBIT to TEa.G3 transition and the current OPL-only release. Historical references are context, not a statement that those runtimes are still part of the product.
Historical release notes and phase reports are evidence of what happened at the time; do not rewrite them
to make old builds appear to have carried current fixes.

## Regression chain: v1.2.1 to v1.2.4

The historical notes record a sequence in which a fix appeared plausible but broke or failed to restore a
feature: achievements caused freezes at scene loads, the next build stopped reporting unlocks, a later build
identified the wrong file/argument passed by the launcher, and the in-game menu exposed a controller-read
freeze. Treat this as a regression case study, not proof that the current code has the same defect.

For every change touching launch hand-off, DMA/SIF, pad interrupts, IGR, memory cards, runtime loading or
storage reads:

1. Add a focused host fixture or structural regression test for the failure mode before changing the code.
2. Couple the test to the actual contract (argument layout, address bounds, channel bit, return path), not
   merely to a function name or a successful compile.
3. Run the full PS2 build and preserve its logs and artifacts; do not skip or weaken a failing check to get
   a green workflow.
4. Keep a separate physical-console acceptance item for behavior that host CI cannot reproduce. Record the
   console model/revision, TEa.G3 commit, pinned OPL build ID, game serial, source, engine, observed result,
   launcher/debug log and a photo or video when a visual symptom matters.

A host fixture proves the tested logic under its fixture assumptions. It does not prove an actual game boot,
sustained reads, stable IGR, controller behavior or memory-card compatibility on hardware.

## PCSX2 is diagnostic, not a performance reference

PCSX2 is useful for repeatable correctness checks, menu navigation, crash reproduction and log collection.
The historical phase-0 measurements show that its modeled GS draw timing does not track the real PS2 GS:
resolution-dependent clear costs that differ materially on the console were nearly constant in the emulator.
Therefore:

- Never publish FPS, frame-time, no-stutter or GS-performance claims derived from PCSX2.
- Performance claims in a release note require COP0/GS timing captured on a physical PS2, with console model,
  video mode, commit and raw measurement log attached.
- The previous baseline uses SCPH-75001; another model may be used only when clearly recorded and never
  represented as a SCPH-75001 result.
- Preserve docs/phase0-results.md and docs/image-quality.md as historical engineering evidence; do not change
  their measured values during a branding cleanup.

## Features whose status must stay explicit

- Virtual memory cards: phase 14 recorded a PCSX2 hang at the first mcGetInfo call with -mc0, while console
  creation/list/read checks were a different test. Do not claim in-game VMC works until physical-PS2 game
  save/load acceptance is recorded.
- RetroAchievements agent: phase 16b recorded end-to-end PCSX2 telemetry and explicitly left the console
  pending. Host and emulator checks do not close the hardware gate.
- IGR and menu: phase 13 had one successful SCPH-75001 Black/480p test, but changes to OPL IGR, GSM, stack layout, kernel patching or menu code require rechecking the affected path on hardware.
- Themes/image quality: only report measured performance for themes and modes with raw console data.
  'Ran without visible stutter' is an observation, not a measured frame-rate result.

## Compatibility and history policy

- Keep historical release notes v1.0 through v1.2.14 unchanged. They describe ORBIT/NULD-era assets and URLs
  as they existed then.
- Current TEa.G3 pages, manuals, download controls and current release notes must use the TEa.G3 identity
  and current package. Link the historical GitHub Releases archive separately where appropriate.
- Keep internal names, migration sources, theme keys and deployed server paths when code depends on them.
  A compatibility identifier such as nuld/, orbit/, nuld_catalog.py or the Classic theme key is not by
  itself a user-visible product-brand failure.
- Do not delete old user data or rewrite historical Git tags/releases as part of a rebrand.

## Current status boundary

The current launcher and release package embed OPL as the only PS2 game engine. The Neutrino runtime,
ELF, loader and IOP/EE modules are not built or embedded. The ui/neutrino-igr directory is retained only
as a UI-design and Arabic-asset reference; it is not linked or active.

tools/check_ee_core_headroom.py is a legacy helper for a historical Neutrino ee_core.map.
tests/test_ee_core_headroom.py unit-tests its parser and threshold logic, but the current OPL-only
workflows do not build that Neutrino image or run an actual ee_core.map guard. Do not describe this
unit test as a current runtime memory-headroom check.

The active launcher build verifies the embedded runtime manifest and requires exactly one runtime
asset: OPL.ELF. This is a build-integrity check, not a physical-console acceptance result.

The final acceptance source remains docs/acceptance-tea-g3.md. A release remains prerelease until its
physical-PS2 checks and evidence record are complete.
