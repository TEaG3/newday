# TEa.G3 v1.3.12 — Production release

## Included
- Single user entry point: `TEa.G3.ELF`.
- OPL is the only embedded PS2 game engine.
- Arabic OPL runtime and launcher assets are built from the pinned source revisions.
- USB ISO/ZSO and USBExtreme UL paths are included in the build.
- Production package includes Arabic and English installation guides, OPL license, package manifest and SHA-256 checksums.
- Website release selection now sorts by publication time and validates the exact repository asset URL.
- Site security hardening: Content Security Policy, no dynamic HTML string injection in site scripts, least-privilege GitHub Actions permissions and CodeQL analysis.

## Explicit limitations
- Physical PS2 hardware acceptance tests are intentionally skipped at the user's request. This release does not claim that any hardware-only path has been validated on a console.
- SMB, UDPBD, UDPFS game streaming and MMCE game launching remain disabled.
- Live catalogue monitoring and staged snapshot/diff publication remain disabled; the launcher uses its boot-time catalogue. No automatic live refresh is claimed.

## Validation performed
- ZSO host decoder/scanner fixtures: passed.
- OPL source runtime build: passed.
- OPL-only TEa.G3 launcher build: passed.
- Runtime manifest and integration package validation: passed.
- Website validation and OPL simulator route matrix: passed.
- CodeQL analysis for C/C++, JavaScript/TypeScript and Python: completed successfully.
- Release package ZIP CRC and SHA-256 generation/verification are performed by the release workflow.

This is a stable software release for the validated scope above. Hardware-only behavior remains subject to the user's console testing.
