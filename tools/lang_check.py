#!/usr/bin/env python3
"""Validate Arabic translations, the two-language interface and the Arabic-first launcher configuration."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STR = r'"(?:[^"\\]|\\.)*"'
CALL = re.compile(r'\bL\(\s*((?:' + STR + r'\s*)+)\)')
PAIR = re.compile(r'\{\s*((?:' + STR + r'\s*)+),\s*((?:' + STR + r'\s*)+)\}')
FORMAT = re.compile(
    r'%(?!%)[-+#0 ]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|z|t|j|L)?[diuoxXfFeEgGaAcspn]'
)


def join(lits):
    """Join adjacent C string literals while retaining their escape sequences."""
    return "".join(re.findall(STR, lits)).replace('""', "")


def english():
    out = set()
    for file in sorted(ROOT.glob("*.c")):
        if file.name == "lang_ar.c":
            continue
        source = file.read_text(encoding="utf-8")
        out |= {join(match.group(1)) for match in CALL.finditer(source)}
    return out


def table(filename):
    source = (ROOT / filename).read_text(encoding="utf-8")
    pairs = [(join(match.group(1)), join(match.group(2))) for match in PAIR.finditer(source)]
    keys = [key for key, _ in pairs]
    seen = set()
    duplicates = set()
    for key in keys:
        if key in seen:
            duplicates.add(key)
        seen.add(key)
    duplicates = sorted(duplicates)
    if duplicates:
        for key in duplicates:
            print(f"duplicate translation key ({filename}):", key)
        raise SystemExit(1)
    return dict(pairs)


def initializer_rows(source, symbol):
    """Count first-level rows in a C [LANG_N] initializer, ignoring braces in strings."""
    start = source.find(symbol)
    if start < 0:
        return 0, ""
    eq = source.find("=", start)
    if eq < 0:
        return 0, ""
    i = eq + 1
    while i < len(source) and source[i].isspace():
        i += 1
    if i >= len(source) or source[i] != "{":
        return 0, ""
    body_start = i
    depth = 0
    rows = 0
    in_string = False
    escaped = False
    for i in range(body_start, len(source)):
        ch = source[i]
        if in_string:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
        elif ch == "{":
            if depth == 1:
                rows += 1
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return rows, source[body_start:i + 1]
    return 0, ""


def main():
    failed = False
    en = english()
    for code, filename in (("ar", "lang_ar.c"),):
        translations = table(filename)
        missing = sorted(en - translations.keys())
        extra = sorted(translations.keys() - en)
        bad = [key for key, value in translations.items()
               if FORMAT.findall(key) != FORMAT.findall(value)]
        for key in missing:
            print(f"missing {code}:", key)
        for key in extra:
            print(f"unused {code}:", key)
        for key in bad:
            print(f"printf format differs ({code}):", key, "->", translations[key])
        print(f"{len(en)} English texts, {len(translations)} {code}, "
              f"{len(missing)} missing, {len(extra)} unused")
        failed |= bool(missing or extra or bad)

    # Reject malformed UTF-8 and replacement characters in user-visible source files.
    text_paths = [ROOT / name for name in ("launcher.c", "lang_ar.c", "lang.h",
                  "gfx.c", "gfx.h", "sources.c", "tools/font.py",
                  "tools/generate_igr_arabic.py", "site/i18n.js")]
    encoding_failed = False
    for path in text_paths:
        raw = path.read_bytes()
        try:
            decoded = raw.decode("utf-8", errors="strict")
        except UnicodeDecodeError as exc:
            print(f"FAIL: invalid UTF-8 in {path.relative_to(ROOT)}: {exc}")
            encoding_failed = True
            continue
        if "\uFFFD" in decoded:
            print(f"FAIL: replacement character U+FFFD in {path.relative_to(ROOT)}")
            encoding_failed = True
    failed |= encoding_failed

    launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
    required = {
        "Arabic must be the initial language": "int lang = LANG_AR;" in launcher,
        "unknown or missing language must fall back to Arabic":
            'static int lang_of(const char *s)' in launcher and
            'return !strcasecmp(s, "en") ? LANG_EN' in launcher and
            ': LANG_AR; }' in launcher,
        "Arabic language key must be registered":
            'lang_key[LANG_N] = {"en", "ar"}' in launcher,
        "fresh config must use Arabic": 'ini_parse(&cfg, config_template[LANG_AR]);' in launcher,
        "Arabic config template marker must exist": '"; TEa.G3 - إعدادات عربية\\n"' in launcher,
        "Arabic config template must set idioma=ar": "idioma = ar\\n" in launcher,
        "developer credit must be present":
            'L("Modified and developed by Iraqi developer TEa.G3")' in launcher and
            '{"Modified and developed by Iraqi developer TEa.G3", "تعديل وتطوير المطور العراقي TEa.G3"}' in
            (ROOT / "lang_ar.c").read_text(encoding="utf-8"),
        "Arabic settings include the default engine selector": "محرك التشغيل" in launcher and "W_ENGINE" in launcher,
        "engine preference is loaded from config.ini": 'ini_get(&cfg, "juegos", "motor", "opl")' in launcher,
        "engine preference is saved to config.ini": 'ini_set(&cfg, "juegos", "motor", w_engine_key[w_val[W_ENGINE]])' in launcher,
        "engine choices preserve auto, Neutrino and OPL": 'w_engine_key[3] = {"auto", "neutrino", "opl"}' in launcher,
        "automatic engine label has an Arabic translation": '{"Automatic", "تلقائي"}' in (ROOT / "lang_ar.c").read_text(encoding="utf-8"),
    }
    for description, passed in required.items():
        print(("ok: " if passed else "FAIL: ") + description)
        failed |= not passed

    for symbol in ("view_names[LANG_N]", "vid_labels[LANG_N]", "gc_name_l[LANG_N]",
                   "mc_labels[LANG_N]", "filter_names[LANG_N]", "lab[LANG_N]"):
        rows, body = initializer_rows(launcher, symbol)
        passed = rows == 2 and any("\u0600" <= ch <= "\u06FF" for ch in body)
        print(("ok: " if passed else "FAIL: ") + f"{symbol}: {rows}/2 language rows, Arabic row present={passed}")
        failed |= not passed

    gfx_source = (ROOT / "gfx.c").read_text(encoding="utf-8")
    arabic_forms = {int(value, 16) for value in re.findall(r'AR\(0x([0-9A-Fa-f]{4}),', gfx_source)}
    arabic_supported = arabic_forms | {0x060C, 0x061B, 0x061F}
    arabic_used = {ord(ch) for source in (launcher, (ROOT / "lang_ar.c").read_text(encoding="utf-8"))
                   for ch in source if 0x0600 <= ord(ch) <= 0x06FF}
    unsupported = sorted(arabic_used - arabic_supported)
    combining_marks = sorted({ord(ch) for source in (launcher, (ROOT / "lang_ar.c").read_text(encoding="utf-8"))
                              for ch in source if 0x064B <= ord(ch) <= 0x065F})
    for cp in combining_marks:
        print(f"FAIL: Arabic combining mark U+{cp:04X} needs mark positioning support")
    failed |= bool(combining_marks)
    for cp in unsupported:
        print(f"FAIL: Arabic codepoint U+{cp:04X} is used but has no shaping/font mapping")
    print(("ok: " if not unsupported else "FAIL: ") +
          f"all Arabic UI codepoints are covered ({len(arabic_used)} distinct codepoints)")
    failed |= bool(unsupported)

    neutrino_igr = (ROOT / "third_party" / "neutrino-igr" / "igr.c").read_text(encoding="utf-8")
    neutrino_menu = (ROOT / "third_party" / "neutrino-igr" / "menu.c").read_text(encoding="utf-8")
    neutrino_patch = (ROOT / "third_party" / "neutrino-igr" / "patch_neutrino.py").read_text(encoding="utf-8")
    neutrino_build = (ROOT / "tools" / "build_neutrino.sh").read_text(encoding="utf-8")
    igr_generator = (ROOT / "tools" / "generate_igr_arabic.py").read_text(encoding="utf-8")
    igr_checks = {
        "launcher passes Arabic to the in-game menu": '"-igrlang=ar"' in launcher,
        "Neutrino patch maps Arabic to language 1": 'igr_lang = !strcmp(&argv[i][9], "ar") ? 1 : 0;' in neutrino_patch,
        "Neutrino in-game menu renders Arabic labels": 'eec.IgrLang == 1' in neutrino_menu and "igr_ar_label_pixels" in neutrino_menu,
        "Neutrino achievement notice has Arabic text": "igr_ar_notice_pixels" in neutrino_menu and "IGR_AR_NOTICE_WIDTH" in neutrino_menu,
        "Arabic IGR bitmap is generated from pinned font": 'NOTICE = "تم فتح الإنجاز"' in igr_generator and 'LABELS = ("إعادة تشغيل", "إيقاف التشغيل", "إلغاء")' in igr_generator,
        "Neutrino builder copies the generated Arabic menu header": '"$FORK/menu_arabic.h"' in neutrino_build and "tools/generate_igr_arabic.py" in neutrino_build,
        "PS2SDK strlcpy compatibility is linked into Neutrino": "size_t strlcpy(char *dst, const char *src, size_t size)" in neutrino_igr,
    }
    for description, passed in igr_checks.items():
        print(("ok: " if passed else "FAIL: ") + description)
        failed |= not passed

    igr_codepoints = {ord(ch) for ch in igr_generator if 0x0600 <= ord(ch) <= 0x06FF}
    igr_unsupported = sorted(igr_codepoints - arabic_supported)
    for cp in igr_unsupported:
        print(f"FAIL: IGR Arabic label U+{cp:04X} has no shaping/font mapping")
    print(("ok: " if not igr_unsupported else "FAIL: ") +
          f"all Arabic IGR label codepoints are covered ({len(igr_codepoints)} distinct codepoints)")
    failed |= bool(igr_unsupported)

    font_tool = (ROOT / "tools" / "font.py").read_text(encoding="utf-8")
    font_header = (ROOT / "gfx.h").read_text(encoding="utf-8")
    passed = ("ARABIC_FORMS" in font_tool and "NotoSansArabicUI.ttf" in font_tool and
              "#define GFX_EXTRAS 161" in font_header and
              "#define GFX_GLYPHS (95 + GFX_EXTRAS)" in font_header)
    print(("ok: " if passed else "FAIL: ") + "Arabic glyph atlas is wired into the renderer")
    failed |= not passed

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
