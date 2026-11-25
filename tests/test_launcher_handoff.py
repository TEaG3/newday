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
        # Seven hand-off sites plus the wrapper definition.
        self.assertEqual(len(calls), 8)
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

    def test_release_workflow_uses_the_recovery_wrapper(self):
        self.assertIn("run_loader_recover(8, opl_argv)", self.release_workflow)
        self.assertNotIn("run_loader(8, opl_argv)", self.release_workflow)
        self.assertIn("USBExtreme UL يستخدم OPL فقط", self.release_workflow)

    def test_release_workflow_uses_the_gitlink_pinned_teag3_opl_source(self):
        self.assertIn("repository: TEaG3/OPL", self.release_workflow)
        self.assertIn("git ls-tree HEAD engine/opl", self.release_workflow)
        self.assertIn("ref: ${{ steps.opl-ref.outputs.sha }}", self.release_workflow)
        self.assertIn("submodules: recursive", self.build_workflow)
        self.assertNotIn("repository: ps2homebrew/Open-PS2-Loader", self.release_workflow)

    def test_both_opl_builds_generate_and_verify_complete_arabic(self):
        for workflow in (self.build_workflow, self.release_workflow):
            self.assertIn("check_arabic_translation.py", workflow)
            self.assertIn("make languages", workflow)
            self.assertIn("test -s lng/lang_Arabic.lng", workflow)
            self.assertIn("grep -Fq 'العودة' lng/lang_Arabic.lng", workflow)
            self.assertIn("test -s lng_src/thirdparty/font_Arabic.ttf", workflow)

    def test_lwnbd_script_is_executable_before_make_clean(self):
        # OPL's clean target executes the downloaded helper directly, not through bash.
        for workflow in (self.build_workflow, self.release_workflow):
            download = workflow.index("download_lwNBD.sh -o download_lwNBD.sh")
            chmod = workflow.index("chmod +x download_lwNBD.sh", download)
            executable = workflow.index("test -x download_lwNBD.sh", chmod)
            clean = workflow.index("make clean", executable)
            self.assertLess(download, chmod)
            self.assertLess(chmod, executable)
            self.assertLess(executable, clean)

    def test_release_opl_build_flags_match_ci(self):
        flags = "make NOT_PACKED=1 RTL=1 PADEMU=1"
        self.assertIn(flags, self.build_workflow)
        self.assertIn(flags, self.release_workflow)
        self.assertIn("test -s include/lang_autogen.h", self.release_workflow)
        self.assertIn("test -s src/lang_internal.c", self.release_workflow)


if __name__ == "__main__":
    unittest.main()
