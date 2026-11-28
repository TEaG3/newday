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


    def test_package_contains_full_bilingual_readmes(self) -> None:
        packager = (ROOT / "tools/package_single_elf_release.py").read_text(encoding="utf-8")
        arabic = (ROOT / "README.md").read_text(encoding="utf-8")
        english = (ROOT / "README-EN.md").read_text(encoding="utf-8")
        self.assertGreater(len(arabic), 1000)
        self.assertGreater(len(english), 1000)
        self.assertIn("README-AR.md", packager)
        self.assertIn("README-EN.md", packager)
        self.assertIn('ROOT / "README-EN.md"', packager)
        self.assertNotIn("README-AR.txt", packager)
        self.assertIn("OPL is the only PS2 game engine", english)
        self.assertIn("does **not** prove", english)

    def test_release_version_is_synchronised(self) -> None:
        launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        self.assertIn('#define TEAG3_VERSION "v1.3.14"', launcher)
        self.assertIn('tag: arabic-v1.3.14', launcher)

    def test_release_write_permission_is_limited_to_publish_job(self) -> None:
        workflow = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        self.assertRegex(workflow, r"(?m)^permissions:\n  contents: read$")
        self.assertRegex(workflow, r"(?s)package-and-release:\n    name: Build single-ELF OPL-only Arabic package\n    permissions:\n      contents: write\n    runs-on:")
        self.assertNotIn("uses: actions/checkout@v4", workflow)
        self.assertNotIn("uses: actions/upload-artifact@v4", workflow)
        self.assertNotIn("uses: actions/download-artifact@v4", workflow)
        self.assertIn("prerelease: false", workflow)

    def test_ps2_toolchain_images_are_pinned_to_immutable_digests(self) -> None:
        workflow = (ROOT / ".github/workflows/build-ps2.yml").read_text(encoding="utf-8")
        self.assertIn(
            "ps2dev/ps2dev@sha256:c6c0c4d633fbe9b2ba9c326d86eddb238039915f5d4728b82c45a2b5f16d9afb",
            workflow,
        )
        self.assertIn(
            "ghcr.io/ps2homebrew/ps2homebrew@sha256:00466c8333dee69d8e5ef7b927580ac3f2e010c6b29375b38d03fec2b7ccec36",
            workflow,
        )
        self.assertNotIn("ps2dev/ps2dev:latest", workflow)
        self.assertNotIn("ghcr.io/ps2homebrew/ps2homebrew:main", workflow)



    def test_release_workflow_uses_the_same_immutable_ps2_toolchain_images(self) -> None:
        workflow = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        self.assertIn(
            "ps2dev/ps2dev@sha256:c6c0c4d633fbe9b2ba9c326d86eddb238039915f5d4728b82c45a2b5f16d9afb",
            workflow,
        )
        self.assertIn(
            "ghcr.io/ps2homebrew/ps2homebrew@sha256:00466c8333dee69d8e5ef7b927580ac3f2e010c6b29375b38d03fec2b7ccec36",
            workflow,
        )
        self.assertNotIn("ps2dev/ps2dev:latest", workflow)
        self.assertNotIn("ghcr.io/ps2homebrew/ps2homebrew:main", workflow)

if __name__ == "__main__":
    unittest.main()
