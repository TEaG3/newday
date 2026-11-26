from __future__ import annotations

import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class NeutrinoUiReferenceTests(unittest.TestCase):
    def test_only_visual_reference_is_retained(self) -> None:
        design = json.loads((ROOT / "ui/neutrino-igr/menu-design.json").read_text(encoding="utf-8"))
        self.assertEqual(design["status"], "reference-only")
        self.assertFalse(design["runtime_linked"])
        self.assertEqual(design["game_engine"], "OPL")
        self.assertEqual([item["id"] for item in design["items"]], ["restart", "power_off", "cancel"])
        for item in design["items"]:
            self.assertTrue(item["en"])
            self.assertRegex(item["ar"], r"[\u0600-\u06FF]")
        self.assertEqual(design["panel"]["height_px"], 54)
        self.assertEqual(design["palette_rgb"]["accent"], [200, 255, 46])
        self.assertEqual(design["achievement_notice"]["duration_ms"], 2500)

    def test_ui_generator_does_not_target_neutrino_runtime_tree(self) -> None:
        generator = (ROOT / "ui/neutrino-igr/generate_arabic_assets.py").read_text(encoding="utf-8")
        self.assertIn('ROOT / "ui" / "neutrino-igr" / "menu_arabic.h"', generator)
        self.assertNotIn('ROOT / "third_party" / "neutrino-igr"', generator)
        self.assertNotIn("build_neutrino.sh", generator)

    def test_reference_is_not_compiled_or_linked_as_a_runtime(self) -> None:
        makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
        workflow = (ROOT / ".github/workflows/build-ps2.yml").read_text(encoding="utf-8")
        self.assertNotIn("ui/neutrino-igr/menu_arabic.h", makefile)
        self.assertNotIn("neutrino.elf", makefile.lower())
        self.assertNotIn("build_neutrino", workflow)
        self.assertNotIn("neutrino.elf", workflow.lower())


if __name__ == "__main__":
    unittest.main()
