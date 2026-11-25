import unittest

from tools.package_single_elf_release import validate_opl_license, validate_release_version


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

    def test_opl_license_uses_its_actual_afl_archive_label(self) -> None:
        notice = '\nAcademic Free License ("AFL") v. 3.0\n'
        self.assertEqual(validate_opl_license(notice), "licenses/OPL-AFL-3.0.txt")

    def test_rejects_unexpected_opl_license_notice(self) -> None:
        with self.assertRaisesRegex(ValueError, "does not declare the expected AFL-3.0"):
            validate_opl_license("GNU GENERAL PUBLIC LICENSE Version 3")


if __name__ == "__main__":
    unittest.main()
