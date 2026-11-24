#!/usr/bin/env python3
"""Bake the NULD fonts (tools/fonts, all SIL OFL 1.1) into font_data.c: one 512-wide PSMT4 alpha atlas + glyph tables.

usage: font.py [OUT.c]  (default: font_data.c next to the Makefile)  |  font.py --selftest
Fonts and sizes: design of openspec change phase-4-orbit-style. Glyph i < 95 is ASCII 32 + i; i >= 95 is EXTRAS[i - 95] (ui: all, title / mono: LATIN_TITLE_CHARS, then FALLBACK).
PSMT4 host layout: even-x pixel in the low nibble (docs/sources.md). CLUT index i = alpha i*128/15 (gfx.c).
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
FONTS = ROOT / "tools" / "fonts"
ASCII = "".join(chr(c) for c in range(32, 127))
LATIN_TITLE_CHARS = "áéíóúñÁÉÍÓÚÑü¿¡·ãõçâêôàÃÕÇÂÊÔÀ"  # legacy Latin characters retained for game titles and metadata
# Unicode Arabic Presentation Forms-B (U+FE80..U+FEFC) are baked as separate contextual glyphs.
# gfx.c shapes Arabic letters into these code points before drawing. Arabic punctuation is also included.
ARABIC_FORMS = "".join(chr(cp) for cp in range(0xFE80, 0xFEFD))
ARABIC_PUNCT = "\u060c\u061b\u061f"  # Arabic comma, semicolon and question mark
LATIN_MORE = "èœŒ"  # retained for European game titles; non-UI fonts fall back to ASCII
ARABIC_CHARS = ARABIC_FORMS + ARABIC_PUNCT
EXTRAS = LATIN_TITLE_CHARS + LATIN_MORE + ARABIC_CHARS
ARABIC_SET = set(ARABIC_CHARS)
ARABIC_FONT = FONTS / "NotoSansArabicUI.ttf"
SPECIAL = {"\u00df": "s", "\u00e6": "a", "\u00c6": "A", "\u0153": "o", "\u0152": "O", "\u00f8": "o", "\u00d8": "O", "\u00f0": "d", "\u00d0": "D",
           "\u00fe": "p", "\u00de": "P", "\u00ab": '"', "\u00bb": '"', "\u00b0": "o", "\u00ba": "o", "\u00aa": "a", "\u00bf": "?", "\u00a1": "!", "\u00b7": "."}
# name, file, px, variation axes in the font's order (None = static font), characters to bake (None = all).
# TEa.G3 interface fonts: Archivo for titles and UI, IBM Plex Mono for data. The logo is rendered
# as a vector mesh by tools/logo.py; the small bitmap logo face is retained for API compatibility.
# Keep the atlas within 512x384 (12 PSMT4 pages); gfx_init allocates it from remaining GS VRAM.
SPECS = (("title", "Archivo.ttf", 38, (700, 125), ASCII + LATIN_TITLE_CHARS),
         ("ui", "Archivo.ttf", 17, (500, 100), None),
         ("mono", "IBMPlexMono-Regular.ttf", 14, None, ASCII + LATIN_TITLE_CHARS),
         ("logo", "Michroma-Regular.ttf", 60, None, "TEa.G3"))
AW, PAGE_H, MAX_PAGES = 512, 128, 12               # PSMT4 page 128x128 -> 4 pages per 128 rows at 512 wide
PAD = 2                                            # blank texels between glyphs (phase-3 known issue)


def fallback(ch):  # the ASCII letter a font without ch draws instead
    import unicodedata
    base = unicodedata.normalize("NFD", ch)[0]
    return SPECIAL.get(ch, base if ord(base) < 127 else "?")


def render(file, px, weight, only, ui=False):
    f = ImageFont.truetype(str(FONTS / file), px)
    if weight:
        f.set_variation_by_axes(list(weight))      # Archivo: wght, wdth
    ar = None
    if ui:
        if not ARABIC_FONT.is_file():
            raise FileNotFoundError(f"{ARABIC_FONT} is missing; run tools/fetch_arabic_font.py first")
        ar = ImageFont.truetype(str(ARABIC_FONT), px)
        # Noto Sans Arabic UI variable-font axes are wdth, wght.
        try:
            axes = ar.get_variation_axes()
            if len(axes) == 2:
                ar.set_variation_by_axes([100, 500])
        except AttributeError:
            pass
    asc, desc = f.getmetrics()
    if ar:
        ar_asc, ar_desc = ar.getmetrics()
        asc, desc = max(asc, ar_asc), max(desc, ar_desc)
    glyphs = []
    for ch in ASCII + EXTRAS:
        if only is not None and ch not in only and ch != " ":
            glyphs.append(dict(img=None, xo=0, yo=0, adv=0))
            continue
        face = ar if ar is not None and ch in ARABIC_SET else f
        x0, y0, x1, y1 = face.getbbox(ch)
        im = Image.new("L", (max(1, x1 - x0), max(1, y1 - y0)))
        ImageDraw.Draw(im).text((-x0, -y0), ch, font=face, fill=255)
        glyphs.append(dict(img=im if x1 > x0 and im.getbbox() else None,  # blank (space): advance only
                           xo=x0, yo=y0, adv=round(face.getlength(ch))))
    return glyphs, asc + desc, asc


def pack(rects):  # rects: list of (w, h); returns [(x, y)] shelf-packed, PAD px apart, and used height
    order = sorted(range(len(rects)), key=lambda i: -rects[i][1])
    pos, x, y, shelf = [None] * len(rects), 0, 0, 0
    for i in order:
        w, h = rects[i]
        if x + w + PAD > AW:
            x, y, shelf = 0, y + shelf + PAD, 0
        pos[i] = (x, y)
        x, shelf = x + w + PAD, max(shelf, h)
    return pos, y + shelf


def build():
    fonts = [(name, *render(file, px, w, only, name == "ui")) for name, file, px, w, only in SPECS]
    items = [g for _, gl, _, _ in fonts for g in gl if g["img"]]
    pos, used = pack([g["img"].size for g in items])
    h = (used + PAGE_H - 1) // PAGE_H * PAGE_H
    assert AW // 128 * h // PAGE_H <= MAX_PAGES, f"atlas {AW}x{h} > {MAX_PAGES} pages"
    atlas = Image.new("L", (AW, h))
    for g, p in zip(items, pos):
        atlas.paste(g["img"], p)
        g["u"], g["v"] = p
    return fonts, atlas, items, pos


def c_source(fonts, atlas):
    a = [round(v * 15 / 255) for v in atlas.tobytes()]
    nib = bytes(a[i] | a[i + 1] << 4 for i in range(0, len(a), 2))
    out = ["// Generated by tools/font.py from Archivo, IBM Plex Mono, Michroma (SIL OFL 1.1, tools/fonts/*OFL.txt). Do not edit.",
           '#include "gfx.h"',
           f"const int font_atlas_h = {atlas.height};",
           "const unsigned short font_extras[GFX_EXTRAS] = {" + ", ".join(str(ord(c)) for c in EXTRAS) + "};",
           "const unsigned char font_fallback[GFX_EXTRAS] = {" + ", ".join(str(ord(fallback(c))) for c in EXTRAS) + "};",
           f"const unsigned char font_atlas[{len(nib)}] __attribute__((aligned(64))) = {{"]
    out += ["\t" + ",".join(str(b) for b in nib[i:i + 32]) + "," for i in range(0, len(nib), 32)]
    out.append("};")
    for name, gl, line_h, asc in fonts:
        out.append(f"static const gfx_glyph {name}_glyphs[GFX_GLYPHS] = {{")
        for g in gl:
            w, h = g["img"].size if g["img"] else (0, 0)
            out.append(f"\t{{{g.get('u', 0)}, {g.get('v', 0)}, {w}, {h}, {g['xo']}, {g['yo']}, {g['adv']}}},")
        out.append("};")
        out.append(f"const gfx_font gfx_font_{name} = {{ {name}_glyphs, {line_h}, {asc} }};")
    return "\n".join(out) + "\n"


def selftest():
    assert len(LATIN_TITLE_CHARS) == 30 and len(ARABIC_FORMS) == 125 and len(ARABIC_PUNCT) == 3
    assert len(EXTRAS) == 161 == len(set(EXTRAS)) and all(0x80 <= ord(c) < 0x10000 for c in EXTRAS)
    assert all(32 <= ord(fallback(c)) < 127 for c in EXTRAS) and fallback("\u00e8") == "e" and fallback("\u0153") == "o"
    assert ARABIC_FONT.is_file(), "Noto Sans Arabic UI font is required (tools/fetch_arabic_font.py)"
    fonts, atlas, items, pos = build()
    r = [(x, y, *g["img"].size) for g, (x, y) in zip(items, pos)]
    for i, (x, y, w, h) in enumerate(r):
        assert x + w <= AW and y + h <= atlas.height and w < 256 and h < 256
        for x2, y2, w2, h2 in r[i + 1:]:
            assert x + w <= x2 or x2 + w2 <= x or y + h <= y2 or y2 + h2 <= y, "overlap"
    for name, gl, _, _ in fonts:
        assert all(-128 <= g["xo"] < 128 and -128 <= g["yo"] < 128 and g["adv"] < 256 for g in gl), name
    print(f"font selftest ok: {len(items)} glyphs, atlas {AW}x{atlas.height} = {AW // 128 * atlas.height // PAGE_H} pages")


if __name__ == "__main__":
    if sys.argv[1:] == ["--selftest"]:
        selftest()
    else:
        fonts, atlas, _, _ = build()
        out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "font_data.c"
        out.write_text(c_source(fonts, atlas), encoding="utf-8", newline="\n")
        print(f"{out}: atlas {AW}x{atlas.height}")
