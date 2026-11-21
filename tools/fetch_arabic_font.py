#!/usr/bin/env python3
"""Fetch the pinned SIL-OFL Noto Sans Arabic UI font used by tools/font.py."""
from hashlib import sha1
from pathlib import Path
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent.parent
DEST = ROOT / "tools" / "fonts"
REV = "a559a6efcfed22bf50219f52ecefcf20b9522408"
FILES = (
    (
        "NotoSansArabicUI.ttf",
        f"https://raw.githubusercontent.com/google/fonts/{REV}/ofl/notosansarabicui/NotoSansArabicUI%5Bwdth%2Cwght%5D.ttf",
        "7852c6ee5f44d304c03fa2e95c52563ad4c526a1",
        100_000,
    ),
    (
        "NotoSansArabicUI-OFL.txt",
        f"https://raw.githubusercontent.com/google/fonts/{REV}/ofl/notosansarabicui/OFL.txt",
        None,
        2_000,
    ),
)


def download(name: str, url: str, expected_git_sha: str | None, min_bytes: int) -> None:
    path = DEST / name
    if path.exists() and path.stat().st_size >= min_bytes:
        data = path.read_bytes()
    else:
        request = Request(url, headers={"User-Agent": "TEaG3-NULD-font-builder/1.0"})
        with urlopen(request, timeout=60) as response:
            data = response.read()
        if len(data) < min_bytes:
            raise RuntimeError(f"Downloaded {name} is unexpectedly small ({len(data)} bytes)")
        path.parent.mkdir(parents=True, exist_ok=True)
        temp = path.with_suffix(path.suffix + ".tmp")
        temp.write_bytes(data)
        temp.replace(path)
    if expected_git_sha:
        header = f"blob {len(data)}\\0".encode("ascii")
        actual = sha1(header + data).hexdigest()
        if actual != expected_git_sha:
            path.unlink(missing_ok=True)
            raise RuntimeError(f"SHA-1 mismatch for {name}: expected {expected_git_sha}, got {actual}")
    print(f"{path}: {len(data)} bytes")


if __name__ == "__main__":
    for item in FILES:
        download(*item)
