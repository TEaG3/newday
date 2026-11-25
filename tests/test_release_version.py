import unittest

from tools.package_single_elf_release import validate_release_version


class ReleaseVersionTests(unittest.TestCase):
    SOURCE = '#define TEAG3_VERSION "v1.3.6"'

    def test_accepts_matching_arabic_release_tag(self) -> None:
        self.assertEqual(validate_release_version("arabic-v1.3.6", self.SOURCE), "v1.3.6")

    def test_accepts_matching_plain_release_tag(self) -> None:
        self.assertEqual(validate_release_version("v1.3.6", self.SOURCE), "v1.3.6")

    def test_keeps_development_package_mode(self) -> None:
        self.assertEqual(validate_release_version("development", self.SOURCE), "development")

    def test_rejects_stale_launcher_version(self) -> None:
        with self.assertRaisesRegex(ValueError, "not v1.3.5"):
            validate_release_version("arabic-v1.3.5", self.SOURCE)

    def test_rejects_malformed_release_tag(self) -> None:
        with self.assertRaisesRegex(ValueError, "invalid TEa.G3 release version"):
            validate_release_version("arabic-latest", self.SOURCE)


if __name__ == "__main__":
    unittest.main()
