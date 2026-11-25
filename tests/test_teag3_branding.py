from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]

class TEaG3BrandingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        cls.net = (ROOT / "net.c").read_text(encoding="utf-8")
        cls.sources = (ROOT / "sources.c").read_text(encoding="utf-8")
        cls.gfx_header = (ROOT / "gfx.h").read_text(encoding="utf-8")
        cls.lang_ar = (ROOT / "lang_ar.c").read_text(encoding="utf-8")
        cls.site_i18n = (ROOT / "site/i18n.js").read_text(encoding="utf-8")

    def test_runtime_messages_and_configuration_use_teag3_branding(self):
        stale = ("nuld vmc remote:", "nuld covers:", "nuld %s\\\\n", "nuld update:",
                 "nuld games:", "nuld launch:", "nuld splash:", "nuld window ",
                 "nuld_catalog.py running?", "# written by NULD")
        for item in stale:
            with self.subTest(item=item):
                self.assertNotIn(item, self.launcher)
        start = self.launcher.index("static const char *const config_template[LANG_N]")
        end = self.launcher.index("\\n};", start)
        config = self.launcher[start:end]
        self.assertNotIn("nuld", config.lower())
        self.assertIn("downloadable applications only", config)
        self.assertIn("Classic", config)

    def test_network_identifies_the_current_product(self):
        self.assertIn("User-Agent: TEa.G3", self.net)
        self.assertNotIn("User-Agent: ORBIT", self.net)

    def test_site_package_translation_describes_single_opl_runtime(self):
        self.assertIn("تتضمن الحزمة ملف TEa.G3.ELF ومحرك OPL الوحيد المضمّن", self.site_i18n)
        self.assertIn("tea_g3/runtime/OPL.ELF", self.site_i18n)
        self.assertNotIn("تتضمن الحزمة ملف TEa.G3.ELF ومحركي OPL وNeutrino", self.site_i18n)
        self.assertNotIn("neutrino/     محرك تشغيل الألعاب", self.site_i18n)

    def test_current_runtime_has_no_secondary_engine_symbols(self):
        for name, text in (("launcher", self.launcher), ("sources", self.sources)):
            with self.subTest(name=name):
                self.assertNotRegex(text, r"(?i)\\b(?:neutrino|engine_neutrino|ndir)\\b")

    def test_arabic_catalog_error_matches_the_current_message_key(self):
        self.assertIn("is the TEa.G3 catalog service running?", self.launcher)
        self.assertIn("هل تعمل خدمة فهرسة TEa.G3؟", self.lang_ar)
        self.assertNotIn("nuld_catalog.py يعمل؟", self.lang_ar)

    def test_branding_assets_use_teag3_wordmark(self):
        self.assertNotIn('Michroma 60 px ("NULD" only)', self.gfx_header)
        self.assertIn("Michroma 60 px (TEa.G3 wordmark)", self.gfx_header)

    def test_legacy_boot_and_user_data_paths_are_preserved(self):
        # These paths are retained only to migrate or reuse existing user data.
        for path in ('"/BOOT/ORBIT.ELF"', '"/BOOT/NULD.ELF"',
                     '"%s/VMC/ORBIT.bin"', '"%s/VMC/NULD.bin"',
                     '"%s/nuld"', '"%s/orbit"'):
            with self.subTest(path=path):
                self.assertIn(path, self.launcher)

if __name__ == "__main__":
    unittest.main()
