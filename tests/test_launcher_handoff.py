"""Structural regression tests for returning from a failed ELF hand-off."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LauncherHandoffTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        cls.gfx = (ROOT / "gfx.c").read_text(encoding="utf-8")
        cls.header = (ROOT / "gfx.h").read_text(encoding="utf-8")
        cls.release_workflow = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        cls.build_workflow = (ROOT / ".github/workflows/build-ps2.yml").read_text(encoding="utf-8")

    def test_low_level_loader_is_called_only_by_recovery_wrapper(self):
        # Every caller must pass through the VBlank recovery path.
        self.assertEqual(len(re.findall(r"\brun_loader\s*\(", self.launcher)), 1)
        self.assertIn("static void run_loader_recover(int argc, char **argv)", self.launcher)

    def test_every_launcher_handoff_uses_recovery_wrapper(self):
        calls = re.findall(r"\brun_loader_recover\s*\(", self.launcher)
        # Six active hand-off sites plus the wrapper definition; no Neutrino fallback.
        self.assertEqual(len(calls), 7)
        self.assertIn("if (gfx_resume())", self.launcher)

    def test_resume_reuses_existing_vblank_state(self):
        start = self.gfx.index("int gfx_resume(void)")
        end = self.gfx.index("void gfx_flip(void)", start)
        body = self.gfx[start:end]
        self.assertIn("vsync_sema < 0", body)
        self.assertIn("AddIntcHandler(INTC_VBLANK_S, vsync_handler, 0)", body)
        self.assertIn("EnableIntc(INTC_VBLANK_S)", body)
        self.assertNotIn("gfx_init(", body)

    def test_bios_disc_handoff_also_recovers_display(self):
        self.assertIn('LoadExecPS2(cv[i].disc == D_PS2', self.launcher)
        start = self.launcher.index('LoadExecPS2(cv[i].disc == D_PS2')
        recovery = self.launcher.index('handoff_returned("BIOS");', start)
        return_stmt = self.launcher.index("return;", start)
        self.assertLess(recovery, return_stmt)

    def test_elf_and_bios_returns_share_recovery_function(self):
        self.assertIn('handoff_returned("ELF");', self.launcher)
        start = self.launcher.index("static void handoff_returned(")
        end = self.launcher.index("static void run_loader_recover(", start)
        body = self.launcher[start:end]
        self.assertIn("gfx_resume()", body)
        self.assertIn('L("Could not start")', body)

    def test_resume_is_declared_in_public_header(self):
        self.assertIn("int gfx_resume(void);", self.header)

    def test_all_opl_adapters_use_the_recovery_wrapper(self):
        self.assertIn("if (engine == ENGINE_OPL_UL)", self.launcher)
        self.assertIn("run_loader_recover(8, opl_argv);", self.launcher)
        self.assertIn("if (engine == ENGINE_OPL_BDM)", self.launcher)
        self.assertIn("if (engine == ENGINE_OPL_ETH)", self.launcher)
        self.assertIn("run_loader_recover(15, opl_argv);", self.launcher)
        self.assertIn("if (engine == ENGINE_OPL_HDD)", self.launcher)
        self.assertIn("run_loader_recover(9, opl_argv);", self.launcher)
        self.assertNotIn("argv[argc++] = NEUTRINO", self.launcher)

    def test_engine_policy_has_no_neutrino_fallback(self):
        engine = (ROOT / "engine.c").read_text(encoding="utf-8")
        self.assertIn("ENGINE_REASON_ENGINE_REMOVED", engine)
        self.assertIn("ENGINE_OPL_ETH", engine)
        self.assertIn("ENGINE_OPL_HDD", engine)
        self.assertNotIn("d.engine = ENGINE_NEUTRINO", engine)

    def test_runtime_build_embeds_opl_and_smb_support_only(self):
        self.assertIn("tools/build_smb_support.sh", self.build_workflow)
        self.assertIn("network/modules/smb_ioman.irx", self.build_workflow)
        self.assertNotIn("tools/build_neutrino.sh", self.build_workflow)
        self.assertNotIn("tea-g3-neutrino-", self.build_workflow)
        self.assertNotIn("cp -R dist/neutrino", self.build_workflow)
        self.assertNotIn("tools/build_neutrino.sh", self.release_workflow)
        self.assertNotIn("cp -R dist/neutrino", self.release_workflow)
        self.assertIn("network/modules/smb_ioman.irx", self.release_workflow)

    def test_release_package_contains_both_support_licenses(self):
        package = (ROOT / "tools/package_single_elf_release.py").read_text(encoding="utf-8")
        self.assertIn("runtime-input/OPL-LICENSE.txt", package)
        self.assertIn("runtime-input/network/SMB-LICENSE.txt", package)
        self.assertIn("SMB-support-AFL-3.0.txt", package)
        self.assertIn("OPL is the only PS2 game engine", package)
        self.assertNotIn("diagnostics/neutrino", package)

    def test_release_workflow_uses_the_ci_pinned_teag3_opl_source(self):
        self.assertIn("repository: TEaG3/OPL", self.release_workflow)
        self.assertIn("ref: 46bad5a4a469c51e45eb1ed879fe43c100b8cb7b", self.release_workflow)
        self.assertNotIn("repository: ps2homebrew/Open-PS2-Loader", self.release_workflow)

    def test_release_opl_build_flags_match_ci(self):
        flags = "make NOT_PACKED=1 RTL=1 PADEMU=1"
        self.assertIn(flags, self.build_workflow)
        self.assertIn(flags, self.release_workflow)
        self.assertIn("test -s include/lang_autogen.h", self.release_workflow)
        self.assertIn("test -s src/lang_internal.c", self.release_workflow)


if __name__ == "__main__":
    unittest.main()
