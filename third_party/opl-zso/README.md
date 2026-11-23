# OPL ZSO/LZ4 reader — vendored source

This directory contains the ZSO sector decoder and its LZ4 dependency copied from
[TEaG3/OPL](https://github.com/TEaG3/OPL), branch `main`.

## Source files

- `zso.c`, `zso.h`: OPL's ZISO/ZSO block-index and sector decoder
- `lz4.c`, `lz4.h`: LZ4 implementation included by OPL
- `OPL-LICENSE-AFL-3.0.txt`: OPL project license text

The upstream ZSO implementation declares two host hooks:
`ziso_alloc()` and `read_raw_data()`. Those hooks are not wired to TEa.G3 yet.
The OPL frontend's `src/zso.c` supplies them using OPL's own file descriptors
and HDD sector API; those assumptions cannot be copied directly into TEa.G3,
whose game launch path currently delegates ISO access to Neutrino.

## Integration status

**Vendored for review; not enabled for game launch.** No `.zso` extension is
advertised or scanned as runnable by this commit. A follow-up adapter must:

1. Validate the ZISO header (magic, header size, block size, alignment and total
   byte count) before initializing the decoder.
2. Provide bounded, seek-safe `read_raw_data` and aligned allocation hooks for
   the actual TEa.G3/Neutrino storage backend.
3. Route Neutrino's sector reads through the decoder without changing the
   existing ISO path for ordinary `.iso` games.
4. Add host-side fixtures for raw and compressed blocks, truncation and
   out-of-range reads, then build with PS2SDK and test on hardware.

Do not treat the presence of these source files as ZSO runtime support.

## Licensing

The ZSO integration files are from the OPL project and are covered by its
Academic Free License 3.0; retain the upstream attribution and license notice.
The LZ4 files retain Yann Collet's BSD 2-Clause notice in their source headers.
