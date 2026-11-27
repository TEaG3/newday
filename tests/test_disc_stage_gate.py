"""Source-level guard for disc-stage swaps and heap-owned save icons."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class DiscStageGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        cls.icon_c = (ROOT / "icon.c").read_text(encoding="utf-8")
        cls.icon_h = (ROOT / "icon.h").read_text(encoding="utf-8")

    def test_disc_swap_waits_for_all_background_readers(self):
        self.assertIn("static int background_workers_busy(void)", self.launcher)
        for flag in ("ra_busy", "art_busy", "update_busy", "dl_busy", "icon_busy", "disc_busy", "cover_busy"):
            with self.subTest(flag=flag):
                self.assertIn(flag, self.launcher)
        self.assertIn("if (!disc_new || background_workers_busy() || cover_lock < 0)", self.launcher)
        self.assertIn("if (disc_new && !background_workers_busy())", self.launcher)
        self.assertIn("disc_apply_pending();", self.launcher)

    def test_heap_icon_destruction_reclaims_nested_buffers(self):
        self.assertIn("void icon_destroy(icon *ic);", self.icon_h)
        self.assertIn("void icon_destroy(icon *ic)", self.icon_c)
        self.assertIn("icon_free(ic);", self.icon_c)
        self.assertIn("free(ic);", self.icon_c)
        self.assertIn("icon_destroy(old_icon);", self.launcher)
        self.assertIn("icon_destroy(ic);", self.launcher)
        self.assertIn("icon_destroy(heap);", self.icon_c)
        self.assertIn("icon_destroy(NULL);", self.icon_c)


if __name__ == "__main__":
    unittest.main()
