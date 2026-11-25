import unittest

from tools.check_ee_core_headroom import MIN_HEADROOM, STACK_BASE, check_headroom, parse_ee_core_end


class EeCoreHeadroomTests(unittest.TestCase):
    def test_parses_current_gnu_ld_map_shape(self):
        end = parse_ee_core_end("  0x00091890                        _end = .\n")
        self.assertEqual(end, 0x91890)
        self.assertEqual(check_headroom(end), STACK_BASE - end)

    def test_accepts_exact_minimum_headroom(self):
        self.assertEqual(check_headroom(STACK_BASE - MIN_HEADROOM), MIN_HEADROOM)

    def test_rejects_less_than_one_kibibyte(self):
        with self.assertRaisesRegex(ValueError, "minimum is 1024 bytes"):
            check_headroom(STACK_BASE - MIN_HEADROOM + 1)

    def test_rejects_symbol_at_or_above_stack(self):
        with self.assertRaises(ValueError):
            check_headroom(STACK_BASE)
        with self.assertRaises(ValueError):
            check_headroom(STACK_BASE + 4)

    def test_missing_or_ambiguous_symbol_fails_closed(self):
        with self.assertRaisesRegex(ValueError, "found 0"):
            parse_ee_core_end("no map symbol here\n")
        with self.assertRaisesRegex(ValueError, "found 2"):
            parse_ee_core_end("0x00090000 _end = .\n0x00091000 _end = .\n")


if __name__ == "__main__":
    unittest.main()
