import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = (ROOT / "launcher.c").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")


def function_body(source: str, start: str, end: str) -> str:
    a = source.index(start)
    b = source.index(end, a)
    return source[a:b]


class CatalogLauncherWiringTests(unittest.TestCase):
    def test_ps2_guard_is_linked_into_launcher(self):
        self.assertIn('#include "catalog_guard_ps2.h"', LAUNCHER)
        objects = next(line for line in MAKEFILE.splitlines() if line.startswith("EE_OBJS ="))
        self.assertIn("catalog_stage.o", objects)
        self.assertIn("catalog_guard_ps2.o", objects)

    def test_guard_initializes_before_loader_releases_splash_gate(self):
        init = LAUNCHER.index("catalog_guard_ps2_init(&catalog_live_guard, &catalog_live_context)")
        ready = LAUNCHER.index("stage = 3;", init)
        self.assertLess(init, ready)
        self.assertIn("catalog_live_ready = 1;", LAUNCHER[init:ready])

    def test_render_frame_holds_a_reader_lease(self):
        loop = LAUNCHER.index("for (int f = 0;; f++) {")
        flip = LAUNCHER.index("gfx_flip();", loop)
        release = LAUNCHER.index("catalog_reader_end();", flip)
        reader = LAUNCHER.index("catalog_reader_try_begin()", loop)
        pad = LAUNCHER.index("pad_buttons()", loop)
        self.assertLess(loop, reader)
        self.assertLess(reader, pad)
        self.assertLess(pad, flip)
        self.assertLess(flip, release)

    def test_background_workers_hold_and_release_reader_leases(self):
        begin = function_body(
            LAUNCHER,
            "static int background_work_begin(volatile int *busy)\n{",
            "static void background_work_end(volatile int *busy)\n{",
        )
        end = function_body(
            LAUNCHER,
            "static void background_work_end(volatile int *busy)\n{",
            "static void cover_thread(void *arg)",
        )
        self.assertIn("catalog_reader_wait_begin();", begin)
        self.assertIn("catalog_activity_update(0)", begin)
        self.assertIn("catalog_guard_set_activity(&catalog_live_guard, playing, io_busy)", LAUNCHER)
        self.assertIn("catalog_activity_update(ra_launching)", end)
        self.assertIn("catalog_reader_end();", end)

    def test_handoff_sets_and_clears_gameplay_activity(self):
        quiesce = function_body(LAUNCHER, "static int background_quiesce(void)", "static void background_resume_if_idle(void)")
        resume = function_body(LAUNCHER, "static void background_resume_if_idle(void)", "static void ra_log(")
        self.assertIn("catalog_activity_update(1)", quiesce)
        self.assertIn("catalog_activity_update(0)", resume)

    def test_live_scan_and_apply_are_still_disabled(self):
        self.assertNotIn("catalog_guard_scan_begin(", LAUNCHER)
        self.assertNotIn("catalog_guard_request_apply(", LAUNCHER)


if __name__ == "__main__":
    unittest.main()
