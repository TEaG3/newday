from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NeutrinoGuardrailTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.igr = (ROOT / "third_party/neutrino-igr/igr.c").read_text(encoding="utf-8")
        cls.ra = (ROOT / "third_party/neutrino-igr/ra.c").read_text(encoding="utf-8")
        cls.agent = (ROOT / "third_party/neutrino-igr/raagent/src/main.c").read_text(encoding="utf-8")
        cls.patch = (ROOT / "third_party/neutrino-igr/patch_neutrino.py").read_text(encoding="utf-8")

    def test_kernel_unpatch_checks_both_signatures_before_any_write(self):
        start = self.igr.index("static int kernel_unpatch(void)")
        end = self.igr.index("// Before the reboot", start)
        body = self.igr[start:end]
        guard = body.index("if (eeload_patch == NULL || mem_clear_patch == NULL)")
        first_write = body.index("eeload_patch[1] =")
        self.assertLess(guard, first_write)
        self.assertIn("return 0;", body)
        self.assertIn("return 1;", body)
        call = self.igr.index("if (!kernel_unpatch())")
        handoff = self.igr.index("LoadExecPS2(eec.IgrExitPath", call)
        self.assertLess(call, handoff)
        self.assertIn("STAGE(COLOR_RED)", self.igr[call:handoff])

    def test_ra_snapshot_bounds_are_checked_before_memory_reads(self):
        self.assertIn("n > RA_WATCH_MAX", self.ra)
        self.assertIn("bytes > RA_SNAP_MAX_BYTES", self.ra)
        self.assertIn("(z != 1 && z != 2 && z != 4)", self.ra)
        self.assertIn("off != bytes", self.ra)
        self.assertIn("a > RA_RAM_HIGH - z", self.ra)

    def test_agent_ip_buffer_is_terminated_after_strncpy(self):
        self.assertIn("me[sizeof(me) - 1] = '\\0';", self.agent)

    def test_unknown_igr_language_does_not_alias_to_a_valid_language(self):
        self.assertIn('else if (!strcmp(&argv[i][9], "en")) igr_lang = 0;', self.patch)
        self.assertIn("unsupported language", self.patch)
        self.assertIn('if (!strcmp(&argv[i][9], "ar")) igr_lang = 1;', self.patch)
        self.assertNotIn('igr_lang = !strcmp(&argv[i][9], "ar") ? 1 : 0;', self.patch)


if __name__ == "__main__":
    unittest.main()
