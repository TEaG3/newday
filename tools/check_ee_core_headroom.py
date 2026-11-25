#!/usr/bin/env python3
"""Fail a Neutrino build when ee_core approaches its fixed IGR stack base."""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

STACK_BASE = 0x94000
MIN_HEADROOM = 1024


def parse_ee_core_end(map_text: str) -> int:
    """Read exactly one GNU ld map entry of the form '0x... _end = .' ."""
    matches = re.findall(r"^\s*(0x[0-9a-fA-F]+)\s+_end\s*=", map_text, flags=re.MULTILINE)
    if len(matches) != 1:
        raise ValueError(f"expected exactly one '_end' map symbol, found {len(matches)}")
    return int(matches[0], 16)


def check_headroom(end: int, stack_base: int = STACK_BASE, minimum: int = MIN_HEADROOM) -> int:
    if end < 0 or stack_base <= 0 or minimum < 0:
        raise ValueError("addresses and minimum headroom must be non-negative")
    headroom = stack_base - end
    if headroom < minimum:
        raise ValueError(
            f"ee_core._end=0x{end:08X} leaves {headroom} bytes before "
            f"IGR stack 0x{stack_base:08X}; minimum is {minimum} bytes"
        )
    return headroom


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("map_file", type=Path, help="GNU ld ee_core.map produced by the current build")
    parser.add_argument("--stack-base", type=lambda x: int(x, 0), default=STACK_BASE)
    parser.add_argument("--min-headroom", type=int, default=MIN_HEADROOM)
    args = parser.parse_args()
    try:
        end = parse_ee_core_end(args.map_file.read_text(encoding="utf-8", errors="strict"))
        headroom = check_headroom(end, args.stack_base, args.min_headroom)
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"ee_core headroom check FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"ee_core._end=0x{end:08X}; IGR stack base=0x{args.stack_base:08X}; "
          f"headroom={headroom} bytes (minimum {args.min_headroom}) PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
