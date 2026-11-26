#!/usr/bin/env python3
"""Static regression checks for the Classic theme selector and legacy alias in TEa.G3."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
launcher = (root / "launcher.c").read_text(encoding="utf-8")
checks = {
    "theme is a setup row": "W_THEME" in launcher and "wiz_change(int d)" in launcher,
    "theme is loaded from config.ini": 'theme_of(ini_get(&cfg, "ui", "tema", "torres"))' in launcher,
    "theme changes immediately": "theme_set(*v)" in launcher,
    "theme is saved to config.ini": 'ini_set(&cfg, "ui", "tema", themes[w_val[W_THEME]].key)' in launcher,
    "Classic theme and legacy key remain available": "TH_CLASSIC" in launcher and '"nuld"' in launcher.lower(),
    "Arabic is the first-boot language": 'lang_of(ini_get(&cfg, "ui", "idioma", "ar"))' in launcher,
    "Iraqi developer credit is visible in settings": "Modified and developed by Iraqi developer TEa.G3" in launcher,
}
for label, ok in checks.items():
    print(("ok: " if ok else "FAIL: ") + label)
if not all(checks.values()):
    raise SystemExit(1)
