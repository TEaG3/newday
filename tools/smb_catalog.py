#!/usr/bin/env python3
"""Generate the TEa.G3 SMB game index for a Neutrino SMB share.

Run this on the computer that hosts the SMB share:
    python tools/smb_catalog.py /path/to/share-root

The share root must contain DVD/ and/or CD/. The generated TEAG3.GAMES
contains: relative path<TAB>display title<TAB>serial (serial may be empty).
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

SERIAL_RE = re.compile(r"(?i)([a-z]{4})[_-]([0-9]{3})[.-]?([0-9]{2})")


def serial_from_name(name: str) -> str:
    match = SERIAL_RE.search(name)
    if not match:
        return ""
    return f"{match.group(1).upper()}_{match.group(2)}.{match.group(3)}"


def safe_relative_path(path: str) -> bool:
    try:
        encoded = path.encode("ascii")
    except UnicodeEncodeError:
        return False
    return (
        0 < len(encoded) < 160
        and not path.startswith("/")
        and "\\" not in path
        and ":" not in path
        and ".." not in path
        and all(0x21 <= byte <= 0x7E for byte in encoded)
    )


def display_title(stem: str) -> str:
    title = SERIAL_RE.sub("", stem).strip(" _-.")
    title = re.sub(r"\s+", " ", title)
    if not title:
        title = stem
    return title.encode("utf-8")[:63].decode("utf-8", "ignore")


def build_rows(root: Path) -> list[str]:
    rows: list[str] = []
    for folder in ("DVD", "CD"):
        directory = root / folder
        if not directory.is_dir():
            continue
        for image in sorted(directory.rglob("*")):
            if not image.is_file() or image.suffix.lower() != ".iso":
                continue
            relative = image.relative_to(root).as_posix()
            if not safe_relative_path(relative):
                continue
            title = display_title(image.stem).replace("\t", " ")
            serial = serial_from_name(image.name)
            rows.append(f"{relative}\t{title}\t{serial}")
    return rows


def generate_catalog(root: Path) -> Path:
    root = root.resolve()
    rows = build_rows(root)
    output = root / "TEAG3.GAMES"
    temporary = root / "TEAG3.GAMES.tmp"
    temporary.write_text(
        "# TEa.G3 SMB catalog v1; path<TAB>title<TAB>serial\n"
        + ("\n".join(rows) + "\n" if rows else ""),
        encoding="utf-8",
        newline="\n",
    )
    temporary.replace(output)
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("share_root", type=Path, help="local folder exported by SMB")
    args = parser.parse_args()
    if not args.share_root.is_dir():
        parser.error(f"not a directory: {args.share_root}")
    rows = build_rows(args.share_root)
    output = generate_catalog(args.share_root)
    print(f"{len(rows)} ISO image(s) indexed in {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
