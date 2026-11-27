from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CatalogSourceProbeWiringTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")

    def test_source_roots_are_checked_before_and_after_candidate_scan(self):
        start = self.launcher.index("static int load_games_into(")
        end = self.launcher.index("static void load_games(", start)
        body = self.launcher[start:end]
        self.assertIn("if (!catalog_source_roots_ready()) incomplete = 1;", body)
        self.assertIn(
            "if (!was_cancelled && !catalog_source_roots_ready()) incomplete = 1;",
            body,
        )
        self.assertIn("snapshot_result = was_cancelled ? CATALOG_STAGE_SCAN_CANCELLED : CATALOG_STAGE_SCAN_ERR_SOURCE;", body)

    def test_missing_optional_folder_is_only_empty_when_source_root_is_present(self):
        start = self.launcher.index("static int scan_dir(")
        end = self.launcher.index("static uint32_t catalog_fingerprint_bytes", start)
        body = self.launcher[start:end]
        self.assertIn("errno != ENOENT ||", body)
        self.assertIn("catalog_source_root_probe(src[s].root", body)
        self.assertIn('!strncmp(src[s].root, "hdd", 3)', body)
        self.assertIn("*incomplete = 1;", body)

    def test_special_sources_keep_their_own_presence_handling(self):
        start = self.launcher.index("static int catalog_source_roots_ready(")
        end = self.launcher.index("static int scan_cancelled(", start)
        body = self.launcher[start:end]
        self.assertIn("src[s].type == SRC_HDL", body)
        self.assertIn("src[s].type == SRC_UDPFS && udpfs_cat", body)
        self.assertIn("catalog_source_root_probe(src[s].root", body)


if __name__ == "__main__":
    unittest.main()
