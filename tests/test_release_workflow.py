from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ReleaseWorkflowRegressionTests(unittest.TestCase):
    def test_opl_arabic_back_label_matches_pinned_translation(self) -> None:
        workflow = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        self.assertIn("grep -Fq 'رجوع' lng/lang_Arabic.lng", workflow)
        self.assertNotIn("grep -Fq 'العودة' lng/lang_Arabic.lng", workflow)

    def test_opl_license_is_staged_at_the_packager_expected_path(self) -> None:
        workflow = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        self.assertIn("test -s runtime-input/OPL-LICENSE.txt", workflow)
        self.assertIn("cp runtime-input/OPL-LICENSE.txt OPL-LICENSE.txt", workflow)
        self.assertIn("Stage OPL license for package validation", workflow)

    def test_release_version_is_synchronised(self) -> None:
        launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        self.assertIn('#define TEAG3_VERSION "v1.3.12"', launcher)
        self.assertIn('tag: arabic-v1.3.12', launcher)

    def test_release_write_permission_is_limited_to_publish_job(self) -> None:
        workflow = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        self.assertRegex(workflow, r"(?m)^permissions:\n  contents: read$")
        self.assertRegex(workflow, r"(?s)package-and-release:\n    name: Build single-ELF OPL-only Arabic package\n    permissions:\n      contents: write\n    runs-on:")
        self.assertNotIn("uses: actions/checkout@v4", workflow)
        self.assertNotIn("uses: actions/upload-artifact@v4", workflow)
        self.assertNotIn("uses: actions/download-artifact@v4", workflow)
        self.assertIn("prerelease: false", workflow)


if __name__ == "__main__":
    unittest.main()
