from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.smb_catalog import build_rows, generate_catalog, safe_relative_path, serial_from_name


class SmbCatalogTests(unittest.TestCase):
    def test_serial_normalization(self) -> None:
        self.assertEqual(serial_from_name("SLUS-21376_Game.iso"), "SLUS_213.76")
        self.assertEqual(serial_from_name("Title without ID.iso"), "")

    def test_catalog_lists_iso_files_under_cd_and_dvd(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "DVD" / "Racing").mkdir(parents=True)
            (root / "CD").mkdir()
            (root / "DVD" / "Racing" / "SLUS-21376_Racing.iso").write_bytes(b"iso")
            (root / "CD" / "Disc Two.iso").write_bytes(b"iso")
            (root / "DVD" / "Compressed.zso").write_bytes(b"zso")
            (root / "DVD" / "archive.zip").write_bytes(b"zip")

            rows = build_rows(root)
            self.assertEqual(len(rows), 3)
            self.assertIn("DVD/Racing/SLUS-21376_Racing.iso\tRacing\tSLUS_213.76", rows)
            self.assertIn("CD/Disc Two.iso\tDisc Two\t", rows)
            self.assertIn("DVD/Compressed.zso\tCompressed\t", rows)

            output = generate_catalog(root)
            self.assertTrue(output.exists())
            self.assertIn("TEa.G3 SMB catalog", output.read_text(encoding="utf-8"))

    def test_non_ascii_paths_are_not_exported(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "DVD").mkdir()
            (root / "DVD" / "لعبة.iso").write_bytes(b"iso")
            self.assertEqual(build_rows(root), [])

    def test_path_guard(self) -> None:
        self.assertTrue(safe_relative_path("DVD/Game 1.iso"))
        self.assertFalse(safe_relative_path("../Game.iso"))
        self.assertFalse(safe_relative_path("DVD/اسم.iso"))
        self.assertFalse(safe_relative_path("DVD\\Game.iso"))


if __name__ == "__main__":
    unittest.main()
