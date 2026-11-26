#!/usr/bin/env python3
"""Static checks for the TEa.G3 GitHub Pages site and local assets."""
from __future__ import annotations

from html.parser import HTMLParser
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / "site"
ERRORS: list[str] = []


class PageParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.references: list[tuple[str, str]] = []
        self.download_links: list[str] = []
        self.ids: set[str] = set()

    def handle_starttag(self, tag: str, attrs) -> None:
        values = dict(attrs)
        if "id" in values:
            self.ids.add(values["id"])
        if "data-zip" in values:
            self.download_links.append(values.get("href", ""))
        for key in ("href", "src"):
            value = values.get(key)
            if value:
                self.references.append((key, value))


def fail(message: str) -> None:
    ERRORS.append(message)


def check_html() -> None:
    pages = sorted(SITE.glob("*.html"))
    if not pages:
        fail("No HTML pages found under site/")
        return
    for page in pages:
        parser = PageParser()
        try:
            parser.feed(page.read_text(encoding="utf-8"))
            parser.close()
        except Exception as exc:
            fail(f"{page.relative_to(ROOT)}: HTML parse error: {exc}")
            continue
        for kind, raw in parser.references:
            ref = raw.strip()
            if not ref or ref.startswith(("https://", "http://", "mailto:", "tel:", "data:", "javascript:")):
                continue
            if ref.startswith("#"):
                anchor = ref[1:]
                if anchor and anchor not in parser.ids:
                    fail(f"{page.relative_to(ROOT)}: missing local anchor #{anchor}")
                continue
            path_part = ref.split("#", 1)[0].split("?", 1)[0]
            if not path_part:
                continue
            target = (page.parent / path_part).resolve()
            try:
                target.relative_to(SITE.resolve())
            except ValueError:
                fail(f"{page.relative_to(ROOT)}: {kind} escapes site/: {raw}")
                continue
            if not target.exists():
                fail(f"{page.relative_to(ROOT)}: missing {kind} target {raw}")


def check_svg() -> None:
    for svg in sorted((SITE / "img").glob("*.svg")):
        try:
            root = ET.parse(svg).getroot()
        except Exception as exc:
            fail(f"{svg.relative_to(ROOT)}: invalid SVG: {exc}")
            continue
        if not root.tag.endswith("svg"):
            fail(f"{svg.relative_to(ROOT)}: root element is not svg")
        for required in ("title", "desc"):
            if not any(node.tag.endswith(required) for node in root):
                fail(f"{svg.relative_to(ROOT)}: missing accessible {required}")


def check_required_opl_files() -> None:
    required = [
        "opl-lab.html",
        "opl-demo.js",
        "opl-lab.css",
        "img/opl-runtime-flow.svg",
        "img/opl-ul-route.svg",
        "img/opl-runtime-install.svg",
    ]
    for rel in required:
        if not (SITE / rel).is_file():
            fail(f"Required OPL site file missing: site/{rel}")
    for rel in ("index.html", "manual.html", "opl-lab.html"):
        path = SITE / rel
        if path.exists():
            html = path.read_text(encoding="utf-8")
            if 'href="opl-lab.css"' not in html:
                fail(f"site/{rel}: missing OPL lab stylesheet")
    index = (SITE / "index.html").read_text(encoding="utf-8")
    if 'href="opl-lab.html"' not in index:
        fail("site/index.html: no link to OPL integration lab")
    if "data-opl-form" not in index or "data-opl-result" not in index:
        fail("site/index.html: OPL route simulator missing from home page")


def check_release_download_links() -> None:
    expected = (
        "https://github.com/TEaG3/newday/releases/download/"
        "arabic-v1.3.12/TEa.G3-Complete-Package.zip"
    )
    for rel in ("index.html", "manual.html", "opl-lab.html"):
        path = SITE / rel
        if not path.is_file():
            fail(f"site/{rel}: page with a download control is missing")
            continue
        parser = PageParser()
        parser.feed(path.read_text(encoding="utf-8"))
        parser.close()
        if parser.download_links != [expected]:
            fail(f"site/{rel}: data-zip fallback must point to the verified v1.3.12 package")


def main() -> int:
    check_html()
    check_svg()
    check_required_opl_files()
    check_release_download_links()
    if ERRORS:
        for item in ERRORS:
            print("FAIL:", item)
        print(f"site validation failed: {len(ERRORS)} issue(s)")
        return 1
    print(f"site validation ok: {len(list(SITE.glob('*.html')))} HTML pages, "
          f"{len(list((SITE / 'img').glob('*.svg')))} SVG diagrams and OPL simulator assets")
    return 0


if __name__ == "__main__":
    sys.exit(main())
