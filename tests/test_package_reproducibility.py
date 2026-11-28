from __future__ import annotations

import os
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

from tools.package_single_elf_release import (
    add_archive_file,
    add_archive_text,
    archive_timestamp,
)


class PackageReproducibilityTests(unittest.TestCase):
    def _build_archive(self, output: Path, source: Path) -> None:
        with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            add_archive_file(archive, source, "bin/TEa.G3.ELF")
            add_archive_text(archive, "README-AR.txt", "تعليمات TEa.G3\n")

    def test_package_bytes_do_not_depend_on_checkout_file_mtimes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "TEa.G3.ELF"
            first = root / "first.zip"
            second = root / "second.zip"
            source.write_bytes(b"same deterministic launcher payload")

            os.utime(source, (1700000000, 1700000000))
            self._build_archive(first, source)
            os.utime(source, (1800000000, 1800000000))
            self._build_archive(second, source)

            self.assertEqual(first.read_bytes(), second.read_bytes())
            with zipfile.ZipFile(first) as archive:
                info = archive.getinfo("bin/TEa.G3.ELF")
                self.assertEqual(info.date_time, archive_timestamp())
                self.assertEqual(info.compress_type, zipfile.ZIP_DEFLATED)
                self.assertEqual(info.external_attr >> 16, 0o100644)
                self.assertEqual(archive.read("README-AR.txt"), "تعليمات TEa.G3\n".encode("utf-8"))

    def test_source_date_epoch_is_normalized_to_zip_two_second_resolution(self) -> None:
        with patch.dict(os.environ, {"SOURCE_DATE_EPOCH": "1767225601"}):
            self.assertEqual(archive_timestamp(), (2026, 1, 1, 0, 0, 0))

    def test_invalid_source_date_epoch_is_rejected(self) -> None:
        for value in ("not-an-integer", "999999999999999999999999999999"):
            with self.subTest(value=value):
                with patch.dict(os.environ, {"SOURCE_DATE_EPOCH": value}):
                    with self.assertRaisesRegex(ValueError, "SOURCE_DATE_EPOCH"):
                        archive_timestamp()


if __name__ == "__main__":
    unittest.main()
