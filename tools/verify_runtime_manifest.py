#!/usr/bin/env python3
"""Validate the OPL-only embedded runtime manifest and its content identity."""
from __future__ import annotations

import hashlib
import re
from pathlib import Path, PurePosixPath
import sys
import zlib

EXPECTED_ASSETS = {"OPL.ELF"}


def verify_runtime_manifest(root: Path | str, manifest_path: Path | str | None = None) -> tuple[str, int]:
    root_path = Path(root).resolve(strict=True)
    if not root_path.is_dir():
        raise ValueError("runtime root is not a directory")

    manifest = Path(manifest_path) if manifest_path is not None else Path("runtime-manifest.txt")
    if not manifest.is_absolute():
        manifest = root_path / manifest
    if manifest.is_symlink():
        raise ValueError("runtime manifest must not be a symlink")
    manifest = manifest.resolve(strict=True)
    if not manifest.is_relative_to(root_path) or not manifest.is_file():
        raise ValueError("runtime manifest is missing or outside the runtime root")

    lines = manifest.read_text(encoding="utf-8").splitlines()
    if len(lines) < 3:
        raise ValueError("runtime manifest is incomplete")
    bundle_match = re.fullmatch(r"bundle=([0-9a-f]{16})", lines[0])
    count_match = re.fullmatch(r"files=([1-9][0-9]*)", lines[1])
    if not bundle_match or not count_match:
        raise ValueError("runtime manifest header is malformed")
    expected_bundle = bundle_match.group(1)
    expected_count = int(count_match.group(1))
    if expected_count != len(lines) - 2:
        raise ValueError("runtime manifest file count does not match its entries")

    seen: set[str] = set()
    digest = hashlib.sha256()
    for line_number, line in enumerate(lines[2:], start=3):
        fields = line.split("\t")
        if len(fields) != 3:
            raise ValueError(f"runtime manifest line {line_number} is malformed")
        relative, size_text, crc_text = fields
        pure = PurePosixPath(relative)
        if (not relative or pure.is_absolute() or "\\" in relative or
                any(part in ("", ".", "..") for part in pure.parts)):
            raise ValueError(f"unsafe runtime path on line {line_number}: {relative!r}")
        if relative in seen:
            raise ValueError(f"duplicate runtime path: {relative}")
        seen.add(relative)
        if relative not in EXPECTED_ASSETS:
            raise ValueError(f"unexpected OPL-only runtime asset: {relative}")
        if not re.fullmatch(r"0|[1-9][0-9]*", size_text):
            raise ValueError(f"invalid runtime size on line {line_number}")
        if not re.fullmatch(r"[0-9a-fA-F]{8}", crc_text):
            raise ValueError(f"invalid runtime CRC-32 on line {line_number}")

        path = root_path.joinpath(*pure.parts)
        try:
            resolved = path.resolve(strict=True)
        except (OSError, RuntimeError) as exc:
            raise ValueError(f"runtime asset is missing: {relative}") from exc
        if path.is_symlink() or not resolved.is_relative_to(root_path) or not resolved.is_file():
            raise ValueError(f"runtime asset is missing or escapes bundle: {relative}")
        data = resolved.read_bytes()
        if len(data) != int(size_text):
            raise ValueError(f"runtime size mismatch: {relative}")
        if (zlib.crc32(data) & 0xFFFFFFFF) != int(crc_text, 16):
            raise ValueError(f"runtime CRC-32 mismatch: {relative}")
        digest.update(relative.encode("utf-8") + b"\0" + data)

    if seen != EXPECTED_ASSETS:
        raise ValueError(f"runtime manifest assets are {sorted(seen)}, expected {sorted(EXPECTED_ASSETS)}")
    actual_bundle = digest.hexdigest()[:16]
    if actual_bundle != expected_bundle:
        raise ValueError(f"runtime bundle identity mismatch: {actual_bundle} != {expected_bundle}")
    return actual_bundle, len(seen)


def main() -> int:
    if len(sys.argv) not in (2, 3):
        print("usage: verify_runtime_manifest.py RUNTIME_ROOT [MANIFEST]", file=sys.stderr)
        return 2
    try:
        bundle, count = verify_runtime_manifest(sys.argv[1], sys.argv[2] if len(sys.argv) == 3 else None)
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"runtime manifest verification failed: {exc}", file=sys.stderr)
        return 1
    print(f"Verified {count} OPL runtime asset(s); bundle {bundle} is intact.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
