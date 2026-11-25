from pathlib import Path
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
        cls.neutrino_patch = (ROOT / "third_party/neutrino-igr/patch_neutrino.py").read_text(encoding="utf-8")
        cls.site_i18n = (ROOT / "site/i18n.js").read_text(encoding="utf-8")

    def test_runtime_messages_and_configuration_use_teag3_branding(self):
        stale_runtime_text = (
            'nuld vmc remote:',
            'nuld covers:',
            'nuld %s\\n',
            'nuld update:',
            'nuld games:',
            'nuld launch:',
            'nuld splash:',
            'nuld window ',
            'nuld_catalog.py running?',
            '# written by NULD',
        )
        for text in stale_runtime_text:
            with self.subTest(text=text):
                self.assertNotIn(text, self.launcher)

        start = self.launcher.index("static const char *const config_template[LANG_N]")
        end = self.launcher.index("\n};", start)
        config = self.launcher[start:end]
        self.assertNotIn("nuld", config.lower())
        self.assertIn("server for online covers, updates and achievements", config)
        self.assertIn("Classic", config)
        self.assertIn('if (!strcasecmp(key, "classic")) return TH_NULD;', self.launcher)

    def test_network_identifies_the_current_product(self):
        self.assertIn("User-Agent: TEa.G3", self.net)
        self.assertNotIn("User-Agent: ORBIT", self.net)

    def test_neutrino_usage_and_ra_diagnostics_use_teag3(self):
        self.assertIn('printf("  -igr=<mask>       TEa.G3:', self.neutrino_patch)
        self.assertIn('printf("TEa.G3 ra:', self.neutrino_patch)
        self.assertNotIn('printf("  -igr=<mask>       ORBIT:', self.neutrino_patch)
        self.assertNotIn('printf("ORBIT ra:', self.neutrino_patch)
        self.assertNotIn("NULD", self.neutrino_patch)

    def test_arabic_catalog_error_matches_the_current_message_key(self):
        self.assertIn("is the TEa.G3 catalog service running?", self.launcher)
        self.assertIn("هل تعمل خدمة فهرسة TEa.G3؟", self.lang_ar)
        self.assertNotIn("nuld_catalog.py يعمل؟", self.lang_ar)

    def test_current_site_package_translation_uses_the_real_executable(self):
        self.assertIn("تتضمن الحزمة ملف TEa.G3.ELF ومحرك OPL فقط", self.site_i18n)
        self.assertIn("USB root\\n├── TEa.G3.ELF", self.site_i18n)
        self.assertIn("المجلد الرئيسي لوحدة USB\\n├── TEa.G3.ELF", self.site_i18n)
        self.assertNotIn("تتضمن launcher.elf", self.site_i18n)

    def test_current_runtime_comments_do_not_misidentify_teag3(self):
        for stale in (
            "mass0:/nuld/ra/",
            "a nuld game's bsd-udpfs config",
            "nuld game the agent loaded",
            "so do nuld games",
            "<home>/nuld/red.elf",
        ):
            with self.subTest(stale=stale):
                self.assertNotIn(stale, self.launcher.lower())
        self.assertNotIn("nuld started from an mmce", self.sources.lower())
        self.assertNotIn("only when nuld runs from an mmce", self.sources.lower())
        self.assertIn("TEa.G3 started from an MMCE", self.sources)
        self.assertNotIn('Michroma 60 px ("NULD" only)', self.gfx_header)
        self.assertIn("Michroma 60 px (TEa.G3 wordmark)", self.gfx_header)
        self.assertNotIn("a nuld game", self.neutrino_patch.lower())

    def test_legacy_boot_and_user_data_paths_are_preserved(self):
        # These names are intentionally retained only to migrate or reuse existing user data.
        for path in (
            '"/BOOT/ORBIT.ELF"',
            '"/BOOT/NULD.ELF"',
            '"%s/VMC/ORBIT.bin"',
            '"%s/VMC/NULD.bin"',
            '"%s/nuld"',
            '"%s/orbit"',
        ):
            with self.subTest(path=path):
                self.assertIn(path, self.launcher)


if __name__ == "__main__":
    unittest.main()
