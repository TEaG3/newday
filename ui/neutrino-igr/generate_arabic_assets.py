#!/usr/bin/env python3
"""Generate retained Neutrino IGR UI bitmap assets for a future OPL port."""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
FONT_PATH = ROOT / "tools" / "fonts" / "NotoSansArabicUI.ttf"
OUTPUT = ROOT / "ui" / "neutrino-igr" / "menu_arabic.h"
HEIGHT = 16
BOX_WIDTH = 216
RIGHT_MARGIN = 12
TEXT_LEFT_MIN = 28
MAX_WIDTH = BOX_WIDTH - RIGHT_MARGIN - TEXT_LEFT_MIN

# (isolated, final, initial, medial) Arabic Presentation Forms-B.
FORMS = {
    0x0621: (0xFE80, None, None, None),
    0x0622: (0xFE81, 0xFE82, None, None),
    0x0623: (0xFE83, 0xFE84, None, None),
    0x0624: (0xFE85, 0xFE86, None, None),
    0x0625: (0xFE87, 0xFE88, None, None),
    0x0626: (0xFE89, 0xFE8A, 0xFE8B, 0xFE8C),
    0x0627: (0xFE8D, 0xFE8E, None, None),
    0x0628: (0xFE8F, 0xFE90, 0xFE91, 0xFE92),
    0x0629: (0xFE93, 0xFE94, None, None),
    0x062A: (0xFE95, 0xFE96, 0xFE97, 0xFE98),
    0x062B: (0xFE99, 0xFE9A, 0xFE9B, 0xFE9C),
    0x062C: (0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0),
    0x062D: (0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4),
    0x062E: (0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8),
    0x062F: (0xFEA9, 0xFEAA, None, None),
    0x0630: (0xFEAB, 0xFEAC, None, None),
    0x0631: (0xFEAD, 0xFEAE, None, None),
    0x0632: (0xFEAF, 0xFEB0, None, None),
    0x0633: (0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4),
    0x0634: (0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8),
    0x0635: (0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC),
    0x0636: (0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0),
    0x0637: (0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4),
    0x0638: (0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8),
    0x0639: (0xFEC9, 0xFECA, 0xFECB, 0xFECC),
    0x063A: (0xFECD, 0xFECE, 0xFECF, 0xFED0),
    0x0641: (0xFED1, 0xFED2, 0xFED3, 0xFED4),
    0x0642: (0xFED5, 0xFED6, 0xFED7, 0xFED8),
    0x0643: (0xFED9, 0xFEDA, 0xFEDB, 0xFEDC),
    0x0644: (0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0),
    0x0645: (0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4),
    0x0646: (0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8),
    0x0647: (0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC),
    0x0648: (0xFEED, 0xFEEE, None, None),
    0x0649: (0xFEEF, 0xFEF0, None, None),
    0x064A: (0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4),
}
LABELS = ("إعادة تشغيل", "إيقاف التشغيل", "إلغاء")
NOTICE = "تم فتح الإنجاز"


def shape_and_reverse(text: str) -> str:
    """Return visual left-to-right codepoints, using pre-shaped Arabic glyphs."""
    logical = [ord(ch) for ch in text]
    shaped = []
    for i, cp in enumerate(logical):
        if cp == 0x20:
            shaped.append(cp)
            continue
        forms = FORMS.get(cp)
        if forms is None:
            raise ValueError(f"Arabic menu letter U+{cp:04X} has no shaping entry")
        previous = FORMS.get(logical[i - 1]) if i > 0 and logical[i - 1] != 0x20 else None
        following = FORMS.get(logical[i + 1]) if i + 1 < len(logical) and logical[i + 1] != 0x20 else None
        joins_previous = bool(previous and previous[2] and forms[1])
        joins_next = bool(following and forms[2] and following[1])
        form = forms[3] if joins_previous and joins_next else forms[1] if joins_previous else forms[2] if joins_next else forms[0]
        shaped.append(form)
    return "".join(chr(cp) for cp in reversed(shaped))


def raster(text: str, font: ImageFont.FreeTypeFont) -> Image.Image:
    visual = shape_and_reverse(text)
    advances = [font.getlength(ch) for ch in visual]
    width = max(1, int(round(sum(advances))) + 2)
    metrics = font.getmetrics()
    canvas = Image.new("L", (width, max(HEIGHT, metrics[0] + metrics[1] + 2)), 0)
    draw = ImageDraw.Draw(canvas)
    x = 0.0
    for ch, advance in zip(visual, advances):
        draw.text((round(x), 0), ch, font=font, fill=255)
        x += advance
    bounds = canvas.getbbox()
    if not bounds:
        raise RuntimeError(f"Arabic menu label rendered empty: {text}")
    canvas = canvas.crop(bounds)
    scale = HEIGHT / canvas.height
    resized_width = max(1, int(round(canvas.width * scale)))
    canvas = canvas.resize((resized_width, HEIGHT), Image.Resampling.NEAREST)
    if canvas.width > MAX_WIDTH:
        raise RuntimeError(f"Arabic label is too wide ({canvas.width}px > {MAX_WIDTH}px): {text}")
    return canvas


def pack(image: Image.Image) -> list[list[int]]:
    rows = []
    for y in range(HEIGHT):
        row = [0] * ((MAX_WIDTH + 7) // 8)
        for x in range(image.width):
            if image.getpixel((x, y)) >= 96:
                row[x // 8] |= 0x80 >> (x % 8)
        rows.append(row)
    return rows


def main() -> None:
    if not FONT_PATH.is_file():
        raise FileNotFoundError(f"{FONT_PATH} missing; run tools/fetch_arabic_font.py first")
    font = ImageFont.truetype(str(FONT_PATH), 10, layout_engine=ImageFont.Layout.BASIC)
    try:
        if len(font.get_variation_axes()) == 2:
            font.set_variation_by_axes([100, 600])
    except AttributeError:
        pass

    images = [raster(label, font) for label in LABELS]
    notice = raster(NOTICE, font)
    widths = [image.width for image in images]
    packed = [pack(image) for image in images]
    notice_packed = pack(notice)
    out = [
        "/* Generated by tools/generate_igr_arabic.py from Noto Sans Arabic UI (SIL OFL). */",
        "#pragma once",
        f"#define IGR_AR_BOX_WIDTH {BOX_WIDTH}",
        f"#define IGR_AR_HEIGHT {HEIGHT}",
        f"#define IGR_AR_MAX_WIDTH {MAX_WIDTH}",
        f"#define IGR_AR_BYTES {(MAX_WIDTH + 7) // 8}",
        "static const unsigned short igr_ar_label_width[3] = {" + ", ".join(map(str, widths)) + "};",
        "static const unsigned char igr_ar_label_pixels[3][IGR_AR_HEIGHT][IGR_AR_BYTES] = {",
    ]
    for label_rows in packed:
        out.append("    {")
        for row in label_rows:
            out.append("        {" + ", ".join(f"0x{value:02X}" for value in row) + "},")
        out.append("    },")
    out += ["};", f"#define IGR_AR_NOTICE_WIDTH {notice.width}",
            "static const unsigned char igr_ar_notice_pixels[IGR_AR_HEIGHT][IGR_AR_BYTES] = {"]
    for row in notice_packed:
        out.append("    {" + ", ".join(f"0x{value:02X}" for value in row) + "},")
    out += ["};", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(out), encoding="utf-8", newline="\n")
    print(f"{OUTPUT}: labels {widths}px; Arabic achievement notice {notice.width}px; {HEIGHT}px high")


if __name__ == "__main__":
    main()
