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

    def test_low_level_loader_is_called_only_by_recovery_wrapper(self):
        # Every caller must pass through the VBlank recovery path.
        self.assertEqual(len(re.findall(r"\\brun_loader\\s*\\(", self.launcher)), 1)
        self.assertIn("static void run_loader_recover(int argc, char **argv)", self.launcher)

    def test_every_launcher_handoff_uses_recovery_wrapper(self):
        calls = re.findall(r"\\brun_loader_recover\\s*\\(", self.launcher)
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

    def test_resume_is_declared_in_public_header(self):
        self.assertIn("int gfx_resume(void);", self.header)


if __name__ == "__main__":
    unittest.main()
