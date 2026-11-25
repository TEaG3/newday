from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.nuld_catalog import Catalog


class CatalogAppPathTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name) / "share"
        (self.root / "APPS").mkdir(parents=True)
        self.catalog = Catalog(str(self.root), "unused-hash-tool", str(self.root / "cache.json"))

    def tearDown(self) -> None:
        self.temp.cleanup()

    def add_app(self, name: str, boot: str, payload: bytes = b"ELF") -> None:
        app = self.root / "APPS" / name
        app.mkdir(parents=True)
        (app / "title.cfg").write_text(f"title={name}\nboot={boot}\n", encoding="utf-8")
        target = app / boot
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)

    def test_lists_and_serves_loose_and_nested_apps(self) -> None:
        (self.root / "APPS" / "Tool.ELF").write_bytes(b"loose")
        self.add_app("Retro", "bin/game.elf", b"nested")

        self.assertEqual(
            set(self.catalog.apps()),
            {("Tool.ELF", "Tool"), ("Retro/bin/game.elf", "Retro")},
        )
        status, body = self.catalog.app_get("/app/Retro/bin/game.elf")
        self.assertEqual((status, body), (200, b"nested"))
        status, body = self.catalog.app_get("/app/Tool.ELF")
        self.assertEqual((status, body), (200, b"loose"))

    def test_rejects_parent_traversal_boot_path(self) -> None:
        (self.root / "outside.elf").write_bytes(b"outside-share")
        app = self.root / "APPS" / "Bad"
        app.mkdir()
        (app / "title.cfg").write_text("title=Bad\nboot=../../outside.elf\n", encoding="utf-8")

        self.assertNotIn(("Bad/../../outside.elf", "Bad"), set(self.catalog.apps()))
        self.assertEqual(self.catalog.app_get("/app/Bad/../../outside.elf"), (404, b""))

    def test_rejects_absolute_boot_path(self) -> None:
        app = self.root / "APPS" / "Absolute"
        app.mkdir()
        (app / "title.cfg").write_text("title=Absolute\nboot=/etc/passwd\n", encoding="utf-8")

        self.assertEqual(list(self.catalog.apps()), [])
        self.assertEqual(self.catalog.app_get("/app/Absolute/%2Fetc/passwd"), (404, b""))

    def test_rejects_symlink_escape(self) -> None:
        outside = self.root / "outside.elf"
        outside.write_bytes(b"outside-share")
        link = self.root / "APPS" / "Escape.ELF"
        try:
            link.symlink_to(outside)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are unavailable on this runner")

        self.assertEqual(list(self.catalog.apps()), [])
        self.assertEqual(self.catalog.app_get("/app/Escape.ELF"), (404, b""))

    def test_rejects_symlinked_app_directory_escape(self) -> None:
        outside = self.root / "outside-app"
        outside.mkdir()
        (outside / "title.cfg").write_text("title=Outside\nboot=game.elf\n", encoding="utf-8")
        (outside / "game.elf").write_bytes(b"outside-app")
        link = self.root / "APPS" / "Escape"
        try:
            link.symlink_to(outside, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are unavailable on this runner")

        self.assertEqual(list(self.catalog.apps()), [])
        self.assertEqual(self.catalog.app_get("/app/Escape/game.elf"), (404, b""))


    def test_iso_scan_rejects_symlink_escape(self) -> None:
        (self.root / "DVD").mkdir()
        outside = Path(self.temp.name) / "outside.iso"
        outside.write_bytes(b"outside-share")
        link = self.root / "DVD" / "Escape.iso"
        try:
            link.symlink_to(outside)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are unavailable on this runner")
        self.assertEqual(list(self.catalog.isos()), [])

    def test_art_endpoint_rejects_symlink_escape(self) -> None:
        (self.root / "ART").mkdir()
        outside = Path(self.temp.name) / "secret.bin"
        outside.write_bytes(b"outside-share")
        link = self.root / "ART" / "cover.jpg"
        try:
            link.symlink_to(outside)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are unavailable on this runner")
        self.assertEqual(self.catalog.art_get("/art/cover.jpg"), (404, b""))

    def test_art_endpoint_serves_regular_in_share_file(self) -> None:
        art = self.root / "ART"
        art.mkdir()
        (art / "cover.jpg").write_bytes(b"cover")
        self.assertEqual(self.catalog.art_get("/art/cover.jpg"), (200, b"cover"))


    def test_iso_scan_rejects_symlinked_media_directory_escape(self) -> None:
        outside = Path(self.temp.name) / "outside-dvd"
        outside.mkdir()
        (outside / "Escape.iso").write_bytes(b"outside-share")
        try:
            (self.root / "DVD").symlink_to(outside, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are unavailable on this runner")
        self.assertEqual(list(self.catalog.isos()), [])

    def test_art_endpoint_rejects_symlinked_art_directory_escape(self) -> None:
        outside = Path(self.temp.name) / "outside-art"
        outside.mkdir()
        (outside / "cover.jpg").write_bytes(b"outside-share")
        try:
            (self.root / "ART").symlink_to(outside, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks are unavailable on this runner")
        self.assertEqual(self.catalog.art_get("/art/cover.jpg"), (404, b""))


if __name__ == "__main__":
    unittest.main()
