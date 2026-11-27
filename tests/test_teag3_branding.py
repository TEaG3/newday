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
        cls.site_index = (ROOT / "site/index.html").read_text(encoding="utf-8")
        cls.site_manual = (ROOT / "site/manual.html").read_text(encoding="utf-8")
        cls.site_opl_lab_css = (ROOT / "site/opl-lab.css").read_text(encoding="utf-8")
        cls.acceptance = (ROOT / "docs/acceptance-tea-g3.md").read_text(encoding="utf-8")
        cls.theme_header = (ROOT / "theme.h").read_text(encoding="utf-8")
        cls.themes = (ROOT / "themes.c").read_text(encoding="utf-8")
        cls.splash_header = (ROOT / "splash.h").read_text(encoding="utf-8")
        cls.torres = (ROOT / "torres.c").read_text(encoding="utf-8")

    def test_runtime_messages_and_configuration_use_teag3_branding(self):
        stale = ("nuld vmc remote:", "nuld covers:", "nuld %s\\\\n", "nuld update:",
                 "nuld games:", "nuld launch:", "nuld splash:", "nuld window ",
                 "nuld_catalog.py running?", "# written by NULD")
        for item in stale:
            with self.subTest(item=item):
                self.assertNotIn(item, self.launcher)
        start = self.launcher.index("static const char *const config_template[LANG_N]")
        end = self.launcher.index("\n};", start)
        config = self.launcher[start:end]
        self.assertNotIn("nuld", config.lower())
        self.assertIn("downloadable applications only", config)
        self.assertIn("motor = opl", config)
        self.assertIn("pantalla = 480i", config)

    def test_classic_theme_replaces_nuld_brand_but_migrates_saved_key(self):
        self.assertIn("TH_CLASSIC", self.theme_header)
        self.assertNotIn("TH_NULD", self.theme_header)
        self.assertIn('if (!strcasecmp(key, "classic") || !strcasecmp(key, "nuld")) return TH_CLASSIC;', self.launcher)
        self.assertIn('{"classic", "Classic"', self.themes)
        self.assertIn('if (!strcasecmp(base_key, "nuld")) base_key = "classic";', self.themes)
        self.assertNotIn("NULD_", self.splash_header)
        self.assertNotIn("nuld_bg", self.themes)
        self.assertNotIn("Temas NULD en vivo", self.torres)

    def test_network_identifies_the_current_product(self):
        self.assertIn("User-Agent: TEa.G3", self.net)
        self.assertNotIn("User-Agent: ORBIT", self.net)

    def test_active_site_copy_hides_legacy_brand_but_preserves_data_migration(self):
        self.assertNotIn("NULD", self.site_i18n)
        self.assertNotIn("nuld (legacy key shown as Classic)", self.site_i18n)
        self.assertNotIn("base = nuld", self.site_i18n)
        self.assertIn("Upgrading from an older launcher?", self.site_i18n)
        self.assertIn("from orbit/ or nuld/ into tea_g3/", self.site_i18n)
        self.assertIn("base = classic", self.site_i18n)
        self.assertIn('legacyLanguageKey = "nuld-site-lang"', self.site_i18n)
        self.assertIn('"torres, Classic, cristal, aurora', self.site_i18n)
        self.assertIn("المظهر الكلاسيكي باسم Classic", self.site_i18n)

    def test_site_package_translation_describes_single_opl_runtime(self):
        self.assertIn("تتضمن الحزمة ملف TEa.G3.ELF ومحرك OPL الوحيد المضمّن", self.site_i18n)
        self.assertIn("tea_g3/runtime/OPL.ELF", self.site_i18n)
        self.assertNotIn("تتضمن الحزمة ملف TEa.G3.ELF ومحركي OPL وNeutrino", self.site_i18n)
        self.assertNotIn("neutrino/     محرك تشغيل الألعاب", self.site_i18n)

    def test_current_runtime_has_no_secondary_engine_symbols(self):
        for name, text in (("launcher", self.launcher), ("sources", self.sources)):
            with self.subTest(name=name):
                self.assertNotRegex(text, r"(?i)\b(?:neutrino|engine_neutrino|ndir)\b")

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
                     '"%s/nuld"', '"%s/orbit"'):
            with self.subTest(path=path):
                self.assertIn(path, self.launcher)

    def test_release_archive_label_does_not_claim_historical_legacy_names(self):
        current_label = "Full release archive (includes earlier versions)"
        self.assertIn(current_label, self.site_index)
        self.assertNotIn("historical ORBIT/NULD releases", self.site_index)
        self.assertNotIn("historical ORBIT/NULD releases", self.site_manual)

    def test_active_docs_claim_only_supported_fat32_storage(self):
        for rel in ("README.md", "docs/game-formats.md", "docs/opl-engine-integration.md", "site/manual.html", "INSTALL-AR.txt", "INSTALL-EN.txt"):
            text = (ROOT / rel).read_text(encoding="utf-8")
            self.assertNotIn("exFAT", text)
        self.assertIn("Neutrino's in-game overlay design is retained", self.site_i18n)
        self.assertIn("The Neutrino runtime, ELF, loader and IOP/EE modules are not included.", self.site_i18n)
        self.assertIn("not linked or active in this build", self.site_i18n)
        self.assertNotIn("neutrino.elf", self.site_i18n.lower())
        self.assertNotIn("build_neutrino", self.site_i18n)
        self.assertNotIn("third_party/neutrino-igr", self.site_i18n)

    def test_current_manual_uses_only_supported_languages_and_translates_opl_sections(self):
        self.assertIn("<td>ar, en</td>", self.site_manual)
        self.assertNotIn("<td>ar, en, es, pt</td>", self.site_manual)
        for key in (
            "3 Configuration",
            "4 OPL engine",
            "5 Video output",
            "6 Troubleshooting",
            "OPL engine and source adapters",
            "OPL is the only PS2 game engine",
            "OPL BDM / USB; real-console sustained-read test pending",
            "Unsupported format/source pairs fail closed. TEa.G3 does not silently route them to a different engine. PS1 uses the separate POPStarter path; ELF applications use the application loader.",
        ):
            with self.subTest(key=key):
                self.assertIn(f'"{key}"', self.site_i18n)
        self.assertIn('"TEa.G3 OPL-only manual"', self.site_i18n)

    def test_opl_lab_rtl_selector_matches_the_teag3_language_toggle(self):
        self.assertIn('document.body.classList.toggle("tea-g3-rtl", ar)', self.site_i18n)
        self.assertIn(".tea-g3-rtl .opl-page", self.site_opl_lab_css)
        self.assertNotIn(".nuld-rtl", self.site_opl_lab_css)

    def test_acceptance_record_matches_current_opl_only_and_monitoring_state(self):
        self.assertIn("### الإصدار المنشور arabic-v1.3.13", self.acceptance)
        self.assertIn("سجل تاريخي: arabic-v1.3.12", self.acceptance)
        self.assertIn("arabic-v1.3.11", self.acceptance)
        self.assertIn("5d6c189ab8661b8f40824193842b322ec6061cb4", self.acceptance)
        self.assertIn("PR #140", self.acceptance)
        self.assertIn("items_capacity", self.acceptance)
        self.assertIn("OPL هو محرك ألعاب PS2 الوحيد", self.acceptance)
        self.assertIn("المراقبة الحية غير مفعّلة", self.acceptance)
        self.assertIn("326/326 سلسلة عربية", self.acceptance)
        self.assertNotIn("دمج OPL وNeutrino داخل نقطة تشغيل واحدة", self.acceptance)
        self.assertNotIn("Neutrino يفتح اللعبة", self.acceptance)


    def test_site_arabic_video_and_usb_copy_guidance_matches_current_build(self):
        self.assertIn("يكون إخراج الفيديو الافتراضي 480i/576i عبر AV وS-Video وRGB SCART وComponent.", self.site_i18n)
        self.assertIn("يبدّل الاختصار L2+R2 إلى 720p للشاشات المتوافقة مع Component/HDMI", self.site_i18n)
        self.assertNotIn("نمط الفيديو (480p أو 1080i)، وخيارات توافق OPL للألعاب التي تحتاج إليها", self.site_i18n)
        self.assertIn("Copy the zip's contents to the root of a FAT32 drive.", self.site_i18n)
        self.assertIn("جذر وحدة تخزين مهيأة بنظام FAT32", self.site_i18n)
        self.assertNotIn("FAT32 or FAT32 drive", self.site_i18n)


if __name__ == "__main__":
    unittest.main()
