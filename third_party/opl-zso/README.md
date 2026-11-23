# OPL ZSO/LZ4 reader — vendored source

This directory contains the ZSO sector decoder and its LZ4 dependency copied from
[TEaG3/OPL](https://github.com/TEaG3/OPL), branch `main`.

## Source files

- `zso.c`, `zso.h`: OPL's ZISO/ZSO block-index and sector decoder
- `lz4.c`, `lz4.h`: LZ4 implementation included by OPL
- `OPL-LICENSE-AFL-3.0.txt`: OPL project license text

## Runtime boundary

**Vendored only; not enabled for game launch.** The OPL decoder expects frontend hooks
`ziso_alloc()` and `read_raw_data()`. OPL links its ISOFS reader into its own IOP
CDVD/device stack. TEa.G3 currently launches PS2 games through Neutrino's FHI and
CDVD emulation path.

Neutrino's current external core does not decode ZSO. The OPL source files cannot be
made functional merely by adding them to the EE launcher or by changing the scanner
to accept `.zso`. A correct implementation needs a deliberate runtime choice:

1. **Neutrino integration:** implement ZSO header/index handling and bounded LZ4
   sector decompression in Neutrino's IOP sector/FHI path, keeping raw ISO reads
   unchanged; or
2. **OPL native backend:** port the required OPL CDVD/ISOFS and source-device path
   as a separate launch backend, then route ZSO titles to it rather than Neutrino.

The second route is broader than these four files: OPL's source tree has multiple
EE/IOP modules and per-device build variants. SMB and BDM/HDD support must be
integrated and packaged separately as well.

## Required safety and correctness tests

Before exposing ZSO as launchable:
- validate magic, header size, block size, alignment, total byte count and index bounds;
- reject truncated files, invalid block offsets, zero/oversized compressed blocks and out-of-range sector requests;
- use bounded LZ4 decompression (the vendored upstream fast decoder may read beyond the compressed input on malformed data);
- test mixed compressed and raw blocks, unaligned reads and multi-sector reads;
- preserve ordinary ISO behavior and test on PS2 hardware.

Do not treat the presence of these source files as ZSO runtime support.

## Licensing

The ZSO integration files are from the OPL project and are covered by its
Academic Free License 3.0; retain the upstream attribution and license notice.
The LZ4 files retain Yann Collet's BSD 2-Clause notice in their source headers.
