"""Structural tests for the OPL-only launcher and AV-safe display defaults."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]

class OplOnlyArchitectureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        cls.engine = (ROOT / "engine.c").read_text(encoding="utf-8")
        cls.engine_h = (ROOT / "engine.h").read_text(encoding="utf-8")
        cls.sources = (ROOT / "sources.c").read_text(encoding="utf-8")
        cls.build = (ROOT / ".github/workflows/build-ps2.yml").read_text(encoding="utf-8")
        cls.release = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        cls.embed = (ROOT / "tools/embed_runtime.py").read_text(encoding="utf-8")
        cls.package = (ROOT / "tools/package_single_elf_release.py").read_text(encoding="utf-8")
        cls.gfx = (ROOT / "gfx.c").read_text(encoding="utf-8")

    def test_no_secondary_runtime_or_legacy_handoff(self):
        for name, text in (("launcher", self.launcher), ("engine", self.engine),
                           ("engine header", self.engine_h), ("sources", self.sources),
                           ("build workflow", self.build), ("release workflow", self.release),
                           ("runtime embedder", self.embed), ("package script", self.package)):
            with self.subTest(name=name):
                self.assertNotIn("engine_neutrino", text.lower())
                self.assertNotRegex(text, r"(?i)\bndir\b")

    def test_engine_policy_fails_closed_without_opl_adapter(self):
        for token in ("ENGINE_UNSUPPORTED", "ENGINE_OPL_BDM", "ENGINE_OPL_HDD", "ENGINE_OPL_UL"):
            self.assertIn(token, self.engine_h)
        self.assertIn("ENGINE_SOURCE_OTHER", self.engine_h)
        self.assertIn("ENGINE_REASON_OPL_RUNTIME_MISSING", self.engine)
        self.assertIn('else if (st == SRC_ILINK) driver = "sd";', self.launcher)
        self.assertIn("No OPL adapter is available", self.launcher)

    def test_network_game_sources_fail_closed(self):
        self.assertNotIn("smb_ioman.irx", self.sources)
        self.assertNotIn("udpbd.irx", self.sources)
        self.assertIn("SMB discovery is disabled", self.sources)
        self.assertIn("UDPFS game streaming is not supported", self.sources)

    def test_remote_catalog_apps_are_validated_before_write(self):
        app_fetch = self.launcher.split("static const char *app_fetch", 1)[1].split("static void launch", 1)[0]
        self.assertIn('#include "app_payload.h"', self.launcher)
        self.assertIn("app_payload_is_valid", app_fetch)
        self.assertIn("(unsigned)len != cv[i].size", app_fetch)
        self.assertIn("body < 0", app_fetch)
        self.assertIn("len > max - body", app_fetch)
        self.assertIn("cv[i].size >= 0x7ffff000u", app_fetch)
        self.assertIn("GetMemorySize()", app_fetch)
        self.assertIn("(unsigned)memory_size", app_fetch)
        self.assertLess(app_fetch.index("app_payload_is_valid"), app_fetch.index('fopen(file, "wb")'))

    def test_second_stage_preserves_the_loaded_target_elf(self):
        loader = (ROOT / "loader/loader.c").read_text(encoding="utf-8")
        load = loader.index("SifLoadElf(argv[0], &elf)")
        handoff = loader.index("ExecPS2(", load)
        self.assertLess(load, handoff)
        self.assertNotIn("GetMemorySize()", loader)
        self.assertNotIn("*(volatile u128 *)a = 0", loader)

    def test_only_opl_is_embedded_and_packaged(self):
        self.assertIn('REQUIRED = ("OPL.ELF",)', self.embed)
        self.assertIn('REQUIRED = ("OPL.ELF",)', self.package)
        self.assertIn("OPL-only", self.build)
        self.assertIn("OPL-only", self.release)

    def test_every_loader_handoff_uses_recovery_wrapper(self):
        self.assertEqual(len(re.findall(r"\brun_loader\s*\(", self.launcher)), 1)
        self.assertGreaterEqual(len(re.findall(r"\brun_loader_recover\s*\(", self.launcher)), 4)
        self.assertIn("if (gfx_resume())", self.launcher)

    def test_analog_interlaced_mode_is_boot_default(self):
        self.assertIn("static int crt = 1", self.launcher)
        self.assertIn("pantalla = 480i", self.launcher)
        self.assertIn("set_mode(1);", self.gfx)
        self.assertIn("GRAPH_MODE_INTERLACED", self.gfx)
        self.assertIn("GRAPH_MODE_HDTV_720P", self.gfx)

    def test_catalog_readers_and_background_workers_are_quiesced_before_iop_reset(self):
        self.assertIn('#include "catalog_guard_ps2.h"', self.launcher)
        self.assertIn("catalog_reader_try_begin()", self.launcher)
        self.assertIn("catalog_reader_end();", self.launcher)
        self.assertIn("static int background_quiesce(void)", self.launcher)
        self.assertIn("background_work_begin(&cover_busy)", self.launcher)
        self.assertIn("background_work_begin(&art_busy)", self.launcher)
        self.assertIn("background_work_begin(&dl_busy)", self.launcher)
        launch_start = self.launcher.index("static void launch(int i)")
        gate = self.launcher.index("background_quiesce()", launch_start)
        handoff = self.launcher.index("run_loader_recover", launch_start)
        self.assertLess(gate, handoff)

if __name__ == "__main__":
    unittest.main()
