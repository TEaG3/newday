import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = (ROOT / "launcher.c").read_text(encoding="utf-8")
STAGE = (ROOT / "catalog_stage.c").read_text(encoding="utf-8")
STAGE_HEADER = (ROOT / "catalog_stage.h").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "Makefile").read_text(encoding="utf-8")
WORKFLOW = (ROOT / ".github" / "workflows" / "build-ps2.yml").read_text(encoding="utf-8")


def function_body(source: str, start: str, end: str) -> str:
    a = source.index(start)
    b = source.index(end, a)
    return source[a:b]


class CatalogLauncherWiringTests(unittest.TestCase):
    def test_catalog_core_has_an_enforced_ee_stack_frame_budget(self):
        self.assertIn("-Wframe-larger-than=8192", WORKFLOW)
        self.assertIn("-Werror -Wframe-larger-than=8192", WORKFLOW)
        self.assertIn("catalog_stage.c -o /tmp/catalog_stage_stack_guard.o", WORKFLOW)
        self.assertIn("catalog_guard_ps2.c -o /tmp/catalog_guard_ps2_stack_guard.o", WORKFLOW)
        start = WORKFLOW.index("sh -c '") + len("sh -c '")
        end = WORKFLOW.index("\n          '", start)
        inner = WORKFLOW[start:end]
        self.assertNotIn("'", inner)


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
        self.assertIn("catalog_activity_wait(1)", quiesce)
        self.assertIn("catalog_activity_update(0)", resume)
        self.assertIn("for (int t = 0; t < 150; t++)", LAUNCHER)
        self.assertIn("usleep(10000);", LAUNCHER)

    def test_boot_catalog_captures_validated_stable_source_keys(self):
        load = function_body(LAUNCHER, "static void load_games(", "// ---- disc drive")
        capture = function_body(LAUNCHER, "static int catalog_snapshot_capture(", "static int by_title(")
        self.assertIn("catalog_snapshot_capture(cv, ncv, &catalog_active_snapshot)", load)
        self.assertIn("catalog_active_snapshot_ready = snapshot_status == CATALOG_STAGE_OK", load)
        self.assertIn("src[source_index].type", capture)
        self.assertIn("src[source_index].unit", capture)
        self.assertIn("ul_name_crc32(item->ul_name)", capture)
        self.assertIn("catalog_stage_snapshot_append(snapshot, &record)", capture)
        fingerprint = function_body(LAUNCHER, "static uint32_t catalog_entry_fingerprint(", "static int catalog_snapshot_capture(")
        self.assertIn("if (item->engine != ENGINE_OPL_UL)", fingerprint)
        self.assertIn("catalog_stage_snapshot_validate(snapshot)", capture)


    def test_validated_iso_and_zso_sizes_reach_the_stable_snapshot(self):
        scan = function_body(LAUNCHER, "static int scan_dir(", "// phase 17:")
        capture = function_body(LAUNCHER, "static int catalog_snapshot_capture(", "static int by_title(")
        self.assertIn("uint64_t catalog_size;", LAUNCHER)
        self.assertIn("item->catalog_size = (uint64_t)bytes;", scan)
        self.assertIn("uint64_t size;", STAGE_HEADER)
        self.assertIn("record.size = item->catalog_size ? item->catalog_size : item->size;", capture)
        self.assertIn("record.size = item->hdl_size_kb;", capture)
        self.assertIn("if (!record.size) record.flags |= 0x80u;", capture)

    def test_safe_publish_primitive_is_host_tested_but_not_enabled_in_launcher(self):
        self.assertIn("catalog_stage_snapshot_publish", STAGE)
        self.assertIn("catalog_guard_try_apply(guard)", STAGE)
        self.assertNotIn("catalog_stage_snapshot_publish(", LAUNCHER)

    def test_live_scan_and_apply_are_still_disabled(self):
        self.assertNotIn("catalog_guard_scan_begin(", LAUNCHER)
        self.assertNotIn("catalog_guard_request_apply(", LAUNCHER)


if __name__ == "__main__":
    unittest.main()
