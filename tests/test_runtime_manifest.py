from __future__ import annotations

import hashlib
import tempfile
import unittest
import zlib
from pathlib import Path

from tools.verify_runtime_manifest import verify_runtime_manifest


class RuntimeManifestTests(unittest.TestCase):
    def write_valid(self, root: Path, data: bytes = b"OPL ELF fixture") -> None:
        (root / "OPL.ELF").write_bytes(data)
        bundle = hashlib.sha256(b"OPL.ELF\0" + data).hexdigest()[:16]
        crc = zlib.crc32(data) & 0xFFFFFFFF
        (root / "runtime-manifest.txt").write_text(
            f"bundle={bundle}\nfiles=1\nOPL.ELF\t{len(data)}\t{crc:08x}\n",
            encoding="utf-8",
        )

    def test_accepts_exact_opl_asset_and_identity(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_valid(root)
            bundle, count = verify_runtime_manifest(root)
            self.assertEqual(count, 1)
            self.assertEqual(len(bundle), 16)

    def test_accepts_manifest_outside_runtime_asset_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            root = base / "runtime"
            root.mkdir()
            self.write_valid(root)
            manifest = base / "runtime-manifest.txt"
            manifest.write_bytes((root / "runtime-manifest.txt").read_bytes())
            (root / "runtime-manifest.txt").unlink()
            bundle, count = verify_runtime_manifest(root, manifest)
            self.assertEqual(count, 1)
            self.assertEqual(len(bundle), 16)

    def test_rejects_symlinked_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            root = base / "runtime"
            root.mkdir()
            self.write_valid(root)
            external = base / "external-manifest.txt"
            external.write_bytes((root / "runtime-manifest.txt").read_bytes())
            (root / "runtime-manifest.txt").unlink()
            try:
                (root / "runtime-manifest.txt").symlink_to(external)
            except (OSError, NotImplementedError):
                self.skipTest("symlinks are unavailable on this runner")
            with self.assertRaisesRegex(ValueError, "must not be a symlink"):
                verify_runtime_manifest(root, root / "runtime-manifest.txt")

    def test_rejects_size_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_valid(root)
            path = root / "runtime-manifest.txt"
            path.write_text(path.read_text().replace("\t15\t", "\t14\t"), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "size mismatch"):
                verify_runtime_manifest(root)

    def test_rejects_crc_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_valid(root)
            path = root / "runtime-manifest.txt"
            lines = path.read_text().splitlines()
            path.write_text("\n".join([lines[0], lines[1], "OPL.ELF\t15\t00000000"]) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "CRC-32 mismatch"):
                verify_runtime_manifest(root)

    def test_rejects_bundle_identity_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_valid(root)
            path = root / "runtime-manifest.txt"
            lines = path.read_text().splitlines()
            path.write_text("bundle=0000000000000000\n" + "\n".join(lines[1:]) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "bundle identity mismatch"):
                verify_runtime_manifest(root)

    def test_rejects_duplicate_entries(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_valid(root)
            path = root / "runtime-manifest.txt"
            lines = path.read_text().splitlines()
            path.write_text("\n".join([lines[0], "files=2", lines[2], lines[2]]) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate runtime path"):
                verify_runtime_manifest(root)

    def test_rejects_path_traversal(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_valid(root)
            path = root / "runtime-manifest.txt"
            lines = path.read_text().splitlines()
            path.write_text("\n".join([lines[0], lines[1], "../OPL.ELF\t15\t00000000"]) + "\n",
                            encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "unsafe runtime path"):
                verify_runtime_manifest(root)

    def test_rejects_unexpected_asset_even_when_file_exists(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.write_valid(root)
            (root / "neutrino.elf").write_bytes(b"secondary runtime")
            path = root / "runtime-manifest.txt"
            path.write_text(path.read_text().replace("files=1", "files=2") +
                            "neutrino.elf\t17\t00000000\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "unexpected OPL-only runtime asset"):
                verify_runtime_manifest(root)

    def test_rejects_symlinked_runtime_asset(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            outside = root.parent / (root.name + "-outside.elf")
            outside.write_bytes(b"OPL ELF fixture")
            try:
                (root / "OPL.ELF").symlink_to(outside)
            except (OSError, NotImplementedError):
                outside.unlink(missing_ok=True)
                self.skipTest("symlinks are unavailable on this runner")
            data = outside.read_bytes()
            bundle = hashlib.sha256(b"OPL.ELF\0" + data).hexdigest()[:16]
            crc = zlib.crc32(data) & 0xFFFFFFFF
            (root / "runtime-manifest.txt").write_text(
                f"bundle={bundle}\nfiles=1\nOPL.ELF\t{len(data)}\t{crc:08x}\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "escapes bundle"):
                verify_runtime_manifest(root)
            outside.unlink()


if __name__ == "__main__":
    unittest.main()
