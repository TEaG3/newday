#!/usr/bin/env python3
"""Check that every English launcher text has Portuguese and Arabic translations.
The [lang] tables in launcher.c carry their own translated rows.
Usage: python tools/lang_check.py (exit 1 if any translation is missing/unused)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STR = r'"(?:[^"\\]|\\.)*"'
CALL = re.compile(r'\bL\(\s*(' + STR + r')\s*,\s*((?:' + STR + r'\s*)+)\)')
PAIR = re.compile(r'\{\s*((?:' + STR + r'\s*)+),\s*((?:' + STR + r'\s*)+)\}')


def join(lits):
    """Join adjacent C string literals while retaining their escape sequences."""
    return "".join(re.findall(STR, lits)).replace('""', "")


def english():
    out = set()
    for f in sorted(ROOT.glob("*.c")):
        if f.name in ("lang_pt.c", "lang_ar.c"):
            continue
        s = f.read_text(encoding="utf-8")
        out |= {join(m.group(2)) for m in CALL.finditer(s)}
    return out


def table(filename):
    s = (ROOT / filename).read_text(encoding="utf-8")
    return {join(m.group(1)): join(m.group(2)) for m in PAIR.finditer(s)}


def main():
    en = english()
    failed = False
    for code, filename in (("pt", "lang_pt.c"), ("ar", "lang_ar.c")):
        translations = table(filename)
        missing = sorted(en - translations.keys())
        extra = sorted(translations.keys() - en)
        bad = [e for e in translations if e.count("%") != translations[e].count("%")]
        for e in missing:
            print(f"missing {code}:", e)
        for e in extra:
            print(f"unused {code}:", e)
        for e in bad:
            print(f"printf args differ ({code}):", e, "->", translations[e])
        print(f"{len(en)} English texts, {len(translations)} {code}, {len(missing)} missing, {len(extra)} unused")
        failed |= bool(missing or extra or bad)
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
