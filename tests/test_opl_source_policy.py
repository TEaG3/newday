from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class OplSourcePolicyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.sources = (ROOT / "sources.c").read_text(encoding="utf-8")
        cls.engine = (ROOT / "engine.c").read_text(encoding="utf-8")
        cls.engine_header = (ROOT / "engine.h").read_text(encoding="utf-8")
        cls.launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        cls.formats = (ROOT / "docs/game-formats.md").read_text(encoding="utf-8")
        cls.packager = (ROOT / "tools/package_single_elf_release.py").read_text(encoding="utf-8")

    def test_smb_source_is_explicitly_fail_closed(self) -> None:
        init = self.sources.split("const char *src_init(", 1)[1].split("// HD Loader metadata", 1)[0]
        self.assertIn("src_err[SRC_SMB] = L(\"SMB discovery is disabled", init)
        self.assertNotIn("add(SRC_SMB", init)
        self.assertNotIn("src[nsrc].type = SRC_SMB", init)

    def test_unsupported_network_sources_cannot_select_an_opl_engine(self) -> None:
        self.assertNotIn("ENGINE_SOURCE_SMB", self.engine_header)
        self.assertNotIn("ENGINE_OPL_ETH", self.engine_header)
        self.assertNotIn("case SRC_SMB:", self.launcher)
        self.assertNotIn("case SRC_UDPFS:", self.launcher)
        self.assertNotIn("case SRC_UDPBD:", self.launcher)
        self.assertIn("ENGINE_UNSUPPORTED", self.engine)
        self.assertIn("return d;", self.engine)

    def test_docs_and_user_package_do_not_claim_smb_runtime_support(self) -> None:
        self.assertIn("| SMB / UDPBD / UDPFS / MMCE game source | Network / memory-card media | Disabled |", self.formats)
        self.assertIn("SMB, UDPBD, UDPFS game streaming and MMCE game launching are disabled", self.packager)
        self.assertIn("until their OPL readers pass validation", self.packager)


if __name__ == "__main__":
    unittest.main()
