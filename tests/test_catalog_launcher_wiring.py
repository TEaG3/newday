import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = (ROOT / "launcher.c").read_text(encoding="utf-8")
STAGE = (ROOT / "catalog_stage.c").read_text(encoding="utf-8")
STAGE_HEADER = (ROOT / "catalog_stage.h").read_text(encoding="utf-8")
SOURCES = (ROOT / "sources.c").read_text(encoding="utf-8")
SOURCES_HDL = (ROOT / "sources_hdl.c").read_text(encoding="utf-8")
SOURCES_HEADER = (ROOT / "sources.h").read_text(encoding="utf-8")
GUARD_PS2 = (ROOT / "catalog_guard_ps2.c").read_text(encoding="utf-8")
GUARD_PS2_HEADER = (ROOT / "catalog_guard_ps2.h").read_text(encoding="utf-8")
UL_SOURCE = (ROOT / "ul.c").read_text(encoding="utf-8")
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


    def test_iso_and_ul_source_selftests_are_executed_in_ci(self):
        self.assertIn("cc -std=gnu99 -Wall -Wextra -Werror -DSELFTEST iso.c -o /tmp/iso_selftest", WORKFLOW)
        self.assertIn("/tmp/iso_selftest", WORKFLOW)
        self.assertIn("cc -std=gnu99 -Wall -Wextra -Werror -DSELFTEST ul.c -o /tmp/ul_selftest", WORKFLOW)
        self.assertIn("/tmp/ul_selftest", WORKFLOW)

    def test_active_catalog_entries_are_pointer_backed_for_generation_swap(self):
        self.assertIn("static entry catalog_boot_items[MAXC];", LAUNCHER)
        self.assertIn("static catalog_stage_bundle catalog_active_bundle;", LAUNCHER)
        self.assertIn("#define cv ((entry *)catalog_active_bundle.items)", LAUNCHER)
        self.assertNotIn("static entry *catalog_items", LAUNCHER)
        self.assertNotIn("static entry cv[MAXC];", LAUNCHER)
        self.assertNotIn("sizeof(cv)", LAUNCHER)
        self.assertNotIn("sizeof cv", LAUNCHER)

    def test_active_bundle_binds_items_sidecars_count_and_snapshot(self):
        init = function_body(LAUNCHER, "static void catalog_bundle_init(void)", "static void scan_cards(void)")
        load = function_body(LAUNCHER, "static void load_games(void)", "// ---- disc drive")
        main = LAUNCHER[LAUNCHER.index("int main(int argc, char *argv[])"):]
        self.assertIn("catalog_active_bundle.items = catalog_boot_items;", init)
        self.assertIn("catalog_active_bundle.items_capacity = MAXC;", init)
        self.assertIn("catalog_active_bundle.sidecars = &catalog_sidecar_boot;", init)
        self.assertIn("catalog_active_bundle.sidecars_capacity = CATALOG_STAGE_MAX_ITEMS;", init)
        self.assertIn("catalog_active_bundle.snapshot = &catalog_boot_snapshot;", init)
        self.assertIn("#define catalog_sidecars ((catalog_stage_sidecar_set *)catalog_active_bundle.sidecars)", LAUNCHER)
        self.assertIn("#define cover_big  (catalog_sidecars->cover_big)", LAUNCHER)
        self.assertIn("#define cover_small (catalog_sidecars->cover_small)", LAUNCHER)
        self.assertIn("#define cover_half (catalog_sidecars->cover_half)", LAUNCHER)
        self.assertIn("void *cover_big[CATALOG_STAGE_MAX_ITEMS];", STAGE_HEADER)
        self.assertIn("void *cover_small[CATALOG_STAGE_MAX_ITEMS];", STAGE_HEADER)
        self.assertIn("void *cover_half[CATALOG_STAGE_MAX_ITEMS];", STAGE_HEADER)
        self.assertNotIn("void *big, *small, *half;", LAUNCHER)
        self.assertIn("void **slot[3] = {&cover_half[i], &cover_small[i], &cover_big[i]};", LAUNCHER)
        self.assertIn("im[0] = cover_half[i], im[1] = cover_small[i], im[2] = cover_big[i];", LAUNCHER)
        self.assertIn("catalog_active_bundle.count = (size_t)count;", load)
        self.assertIn("#define ncv ((int)catalog_active_bundle.count)", LAUNCHER)
        self.assertNotIn("ncv = count;", load)
        self.assertLess(main.index("catalog_bundle_init();"), main.index("StartThread(tid, NULL);"))

    def test_worker_request_indices_are_owned_by_the_active_generation(self):
        for name in ("icon_want", "vmc_net_want", "ra_want", "ra_list_want", "ra_list_for"):
            with self.subTest(name=name):
                self.assertRegex(LAUNCHER, rf"(?m)^#define {name}\s+\(catalog_sidecars->{name}\)$")
                self.assertNotIn(f"static volatile int {name}", LAUNCHER)
        init = function_body(LAUNCHER, "static void catalog_bundle_init(void)", "static void scan_cards(void)")
        self.assertIn("catalog_stage_sidecar_set_discard(&catalog_sidecar_boot);", init)
        self.assertIn("state->icon_want = -1;", STAGE)
        self.assertIn("sidecar_rebase_pending_index", STAGE)

    def test_bundle_selection_is_saved_and_restored_between_frames(self):
        init = function_body(LAUNCHER, "static void catalog_bundle_init(void)", "static void scan_cards(void)")
        load = function_body(LAUNCHER, "static void load_games(void)", "// ---- disc drive")
        main = LAUNCHER[LAUNCHER.index("int main(int argc, char *argv[])"):]
        self.assertIn("int selected_index;", STAGE_HEADER)
        self.assertIn("catalog_active_bundle.selected_index = -1;", init)
        self.assertIn("catalog_active_bundle.selected_index = count > 0 ? 0 : -1;", load)
        self.assertIn("catalog_active_bundle.selected_index = ncv > 0 && sel >= 0 && sel < ncv ? sel : -1;", main)
        self.assertIn("catalog_active_bundle.selected_index >= 0", main)
        self.assertIn("candidate->selected_index = next_selected;", STAGE)
        self.assertIn("catalog_stage_snapshot_map(active->snapshot, candidate->snapshot", STAGE)

    def test_ps2_guard_is_linked_into_launcher(self):
        self.assertIn('#include "catalog_guard_ps2.h"', LAUNCHER)
        objects = next(line for line in MAKEFILE.splitlines() if line.startswith("EE_OBJS ="))
        self.assertIn("catalog_stage.o", objects)
        self.assertIn("catalog_guard_ps2.o", objects)
        self.assertIn("sources_hdl.o", objects)

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

    def test_icon_worker_keeps_storage_only_io_outside_catalog_reader_lease(self):
        io_begin = function_body(
            LAUNCHER,
            "static int background_io_begin(volatile int *busy)\n{",
            "static void background_io_end(volatile int *busy)\n{",
        )
        io_end = function_body(
            LAUNCHER,
            "static void background_io_end(volatile int *busy)\n{",
            "static void cover_thread(void *arg)",
        )
        self.assertNotIn("catalog_reader_wait_begin()", io_begin)
        self.assertNotIn("catalog_reader_end()", io_end)
        self.assertIn("*busy = 1;", io_begin)
        self.assertIn("catalog_activity_update(0)", io_begin)
        self.assertIn("catalog_activity_update(ra_launching)", io_end)
        self.assertIn("(void)catalog_activity_update(0);", io_begin)
        self.assertIn("(void)catalog_activity_update(ra_launching);", io_end)
        begin_lock = io_begin.index("WaitSema(cover_lock)")
        begin_update = io_begin.index("catalog_activity_update(0)")
        begin_unlock = io_begin.index("SignalSema(cover_lock)")
        self.assertLess(begin_lock, begin_update)
        self.assertLess(begin_update, begin_unlock)
        self.assertLess(begin_unlock, io_begin.index("usleep(1000)"))
        end_lock = io_end.index("WaitSema(cover_lock)")
        end_update = io_end.index("catalog_activity_update(ra_launching)")
        end_unlock = io_end.index("SignalSema(cover_lock)")
        self.assertLess(end_lock, end_update)
        self.assertLess(end_update, end_unlock)
        self.assertLess(end_unlock, io_end.index("usleep(1000)"))

        icon = function_body(LAUNCHER, "static void icon_thread(void *arg)", "// ---- covers ----")
        begin_io = icon.index("background_io_begin(&icon_busy)")
        stub = icon.index("install_stub()")
        wizard = icon.index("wiz_job()")
        end_io = icon.index("background_io_end(&icon_busy)")
        begin_catalog = icon.index("background_work_begin(&icon_busy)")
        read_index = icon.index("int i = icon_want, port;")
        read_sidecar = icon.index("vmc_state[i]")
        end_catalog = icon.index("background_work_end(&icon_busy)")
        self.assertLess(begin_io, stub)
        self.assertLess(stub, wizard)
        self.assertLess(wizard, end_io)
        self.assertLess(end_io, begin_catalog)
        self.assertLess(begin_catalog, read_index)
        self.assertLess(read_index, read_sidecar)
        self.assertLess(read_sidecar, end_catalog)
        self.assertIn("if (icon_busy) continue;", icon)

    def test_disc_and_update_workers_do_not_pin_catalog_reader_for_storage_io(self):
        disc = function_body(LAUNCHER, "static void disc_thread(void *arg)\\n{", "// ---- config")
        self.assertIn("background_io_begin(&disc_busy)", disc)
        self.assertIn("catalog_reader_wait_begin();", disc)
        self.assertIn("catalog_reader_end();", disc)
        self.assertIn("background_io_end(&disc_busy);", disc)
        self.assertNotIn("background_work_begin(&disc_busy)", disc)
        self.assertNotIn("background_work_end(&disc_busy)", disc)
        self.assertLess(disc.index("background_io_begin(&disc_busy)"), disc.index("sceCdInit"))
        self.assertLess(disc.index("sceCdGetDiskType"), disc.index("catalog_reader_wait_begin();"))
        self.assertLess(disc.index("catalog_reader_wait_begin();"), disc.index("for (int i = 1; i < ncv"))
        self.assertLess(disc.index("for (int i = 1; i < ncv"), disc.index("catalog_reader_end();"))
        self.assertLess(disc.index("catalog_reader_end();"), disc.index("load_covers("))
        self.assertLess(disc.index("load_covers("), disc.index("background_io_end(&disc_busy);"))

        update = function_body(LAUNCHER, "static void update_check(void)", "static u8 loader_stack")
        self.assertIn("background_io_begin(&update_busy)", update)
        self.assertIn("background_io_end(&update_busy)", update)
        self.assertNotIn("background_work_begin(&update_busy)", update)
        self.assertNotIn("background_work_end(&update_busy)", update)


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
        load = function_body(LAUNCHER, "static int load_games_into(", "static void load_games(void)")
        capture = function_body(LAUNCHER, "static int catalog_snapshot_capture(", "static int by_title(")
        self.assertIn("catalog_snapshot_capture(items, n, snapshot)", load)
        self.assertIn("*snapshot_ready = snapshot_result == CATALOG_STAGE_SCAN_OK", load)
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


    def test_directory_and_app_scans_fail_closed_on_capacity_and_io_errors(self):
        scan = function_body(LAUNCHER, "static int scan_dir(", "// phase 17:")
        self.assertIn("int *incomplete", scan)
        self.assertIn("if (n >= MAXC)", scan)
        self.assertIn("if (n > MAXC)", scan)
        self.assertIn("errno = 0", scan)
        self.assertIn("if (errno != ENOENT)", scan)
        self.assertIn("if (closedir(d) < 0)", scan)
        self.assertIn("fileXioClose(fd) < 0", scan)
        self.assertIn("*incomplete = 1", scan)
        apps = function_body(LAUNCHER, "int apps_path_len =", "qsort(items + 1")
        self.assertIn("if (n >= MAXC)", apps)
        self.assertIn("if (errno != 0) incomplete = 1;", apps)
        self.assertIn("if (d && closedir(d) < 0)", apps)
        self.assertIn("incomplete = 1;", apps)
        builder = function_body(LAUNCHER, "static int load_games_into(", "static void load_games(void)")
        self.assertIn("if (n > MAXC) n = MAXC, incomplete = 1;", builder)

    def test_all_source_scanners_write_only_to_the_supplied_candidate_buffer(self):
        ul = function_body(LAUNCHER, "static int scan_ul(", "static int scan_dir(")
        iso = function_body(LAUNCHER, "static int scan_dir(", "// phase 17:")
        remote = function_body(LAUNCHER, "static int catalog_load(", "typedef struct { int s, n; entry *items; int report_progress; } hdl_ctx;")
        hdl = function_body(LAUNCHER, "static void hdl_add(", "static uint32_t catalog_fingerprint_bytes")
        self.assertIn("entry *items", ul)
        self.assertIn("entry *e = &items[n]", ul)
        self.assertNotIn("&cv[n]", ul)
        self.assertIn("entry *items", iso)
        self.assertIn("entry *item = &items[n]", iso)
        self.assertNotIn("&cv[n]", iso)
        self.assertIn("entry *c = &items[n++]", remote)
        self.assertNotIn("&cv[n", remote)
        self.assertIn("entry *e = &c->items[c->n++]", hdl)
        self.assertIn("memset(e, 0, sizeof(*e))", hdl)

    def test_boot_scan_builds_candidate_before_exposing_active_count(self):
        builder = function_body(LAUNCHER, "static int load_games_into(", "static void load_games(void)")
        publish = function_body(LAUNCHER, "static void load_games(void)", "// ---- disc drive")
        self.assertIn("memset(&items[0], 0, sizeof(items[0]))", builder)
        self.assertIn("scan_dir(items, s, 0, n, &incomplete, cancelled, cancel_context, &source_cancelled)", builder)
        self.assertIn("scan_dir(items, s, 1, n, &incomplete, cancelled, cancel_context, &source_cancelled)", builder)
        self.assertIn("scan_dir(items, 0, 2, n, &incomplete, cancelled, cancel_context, &source_cancelled)", builder)
        self.assertNotIn("scan_dir(items, s, 1, scan_dir", builder)
        self.assertIn("scan_ul(items, s, n, cancelled, cancel_context, &ul_cancelled)", builder)
        self.assertIn("qsort(items + 1", builder)
        self.assertIn("catalog_snapshot_capture(items, n, snapshot)", builder)
        self.assertIn("entry *candidate = malloc(sizeof(entry) * MAXC)", publish)
        self.assertIn("count = load_games_into(candidate, catalog_active_bundle.snapshot, &snapshot_ready, &scan_status, NULL, NULL)", publish)
        self.assertIn("catalog_active_snapshot_ready = snapshot_ready;", publish)
        self.assertLess(publish.index("memcpy(cv, candidate"), publish.index("catalog_active_bundle.count = (size_t)count;"))
        self.assertNotIn("ncv = count;", publish)
        self.assertIn("free(candidate)", publish)
        self.assertIn("load_games_into(cv, catalog_active_bundle.snapshot, &snapshot_ready, &scan_status, NULL, NULL)", publish)

    def test_hdl_scan_rejects_partial_directory_reads_and_closes_before_callbacks(self):
        start = SOURCES_HDL.index("int src_hdl_scan_cancelable(")
        end = SOURCES_HDL.index("\n}", start) + 2
        body = SOURCES_HDL[start:end]
        legacy = function_body(SOURCES_HDL, "int src_hdl_scan(", "\n}")
        self.assertIn("SRC_HDL_SCAN_ERR_OPEN", body)
        self.assertIn("SRC_HDL_SCAN_ERR_READ", body)
        self.assertIn("SRC_HDL_SCAN_ERR_CLOSE", body)
        self.assertEqual(body.count("for (;;) {"), 2)
        self.assertEqual(body.count("read_result = fileXioDread(fd, &de);"), 2)
        self.assertEqual(body.count("if (read_result < 0)"), 2)
        self.assertEqual(body.count("if (read_result == 0) break;"), 2)
        self.assertIn("if (read_result < 0 && status == 0)", body)
        self.assertIn("if (fileXioDclose(fd) < 0", body)
        self.assertIn("hdl_scan_free(head);", body)
        self.assertLess(body.rindex("fileXioDclose(fd)"), body.index("if (cb) cb("))
        self.assertIn("SRC_HDL_SCAN_ERR_MEMORY", SOURCES_HEADER)
        self.assertIn("SRC_HDL_SCAN_ERR_METADATA", SOURCES_HEADER)
        self.assertIn("u64 base_size_kb", body)
        self.assertIn("u64 add_kb64", body)
        self.assertIn("SRC_HDL_SCAN_ERR_CANCELLED", body)
        self.assertIn("hdl_scan_cancelled(cancelled, cancel_context)", body)
        self.assertIn("return src_hdl_scan_cancelable(cb, ctx, NULL, NULL);", legacy)

    def test_hdl_scanner_has_a_real_host_c_fixture(self):
        self.assertIn("sources_hdl.c tests/catalog_host/test_sources_hdl_scan.c", WORKFLOW)
        self.assertIn("/tmp/sources_hdl_test", WORKFLOW)
        self.assertIn("SRC_HDL_SCAN_ERR_READ", SOURCES_HEADER)

    def test_ul_short_reads_and_close_failures_invalidate_the_stable_snapshot(self):
        ul = function_body(LAUNCHER, "static int scan_ul(", "static int scan_dir(")
        self.assertIn("if (got != (int)sizeof(raw))", ul)
        self.assertIn("failed = 1;", ul)
        self.assertIn("if (fileXioClose(fd) < 0) failed = 1;", ul)
        self.assertIn("if (failed)", ul)
        self.assertIn("done_n = previous_done;", ul)
        self.assertIn("total_n = previous_total;", ul)
        self.assertIn("return -1;", ul)
        self.assertIn("return n;", ul)
        load = function_body(LAUNCHER, "static int load_games_into(", "static void load_games(void)")
        self.assertIn("if (hdl_status < 0)", load)
        self.assertIn("if (ul_count < 0)", load)
        self.assertIn("int snapshot_result;", load)
        self.assertIn("snapshot->count = 0;", load)
        self.assertIn("CATALOG_STAGE_SCAN_ERR_SOURCE", load)
        self.assertIn("*snapshot_ready = snapshot_result == CATALOG_STAGE_SCAN_OK", load)

    def test_ul_scan_rejects_valid_records_that_exceed_candidate_capacity(self):
        ul = function_body(LAUNCHER, "static int scan_ul(", "static int scan_dir(")
        self.assertNotIn("n >= MAXC", ul)
        self.assertIn("if (n > MAXC) return -1;", ul)
        self.assertIn("for (int r = 0; r < records; r++)", ul)
        self.assertIn("ul_catalog_capacity_fits(1u, (unsigned)n, MAXC)", ul)
        self.assertIn("failed = 1;", ul)
        self.assertIn("return -1;", ul)
        self.assertIn("incoming <= capacity - existing", UL_SOURCE)
        self.assertIn("assert(!ul_catalog_capacity_fits(1u, 1024u, 1024u));", UL_SOURCE)
        self.assertIn("assert(!ul_catalog_capacity_fits(0u, 1025u, 1024u));", UL_SOURCE)

    def test_safe_publish_primitive_is_host_tested_but_not_enabled_in_launcher(self):
        self.assertIn("catalog_stage_snapshot_publish", STAGE)
        self.assertIn("catalog_guard_try_apply(guard)", STAGE)
        self.assertNotIn("catalog_stage_snapshot_publish(", LAUNCHER)

    def test_bundle_publish_moves_items_sidecars_count_and_snapshot_as_one_generation(self):
        header = STAGE_HEADER
        host_test = (ROOT / "tests" / "catalog_host" / "test_catalog_stage.c").read_text(encoding="utf-8")
        ps2_test = (ROOT / "tests" / "catalog_host" / "test_catalog_guard_ps2.c").read_text(encoding="utf-8")
        self.assertIn("catalog_stage_bundle_publish", STAGE)
        self.assertIn("catalog_stage_bundle", header)
        self.assertIn("test_safe_bundle_publish", host_test)
        self.assertIn("catalog_stage_bundle_publish", ps2_test)
        self.assertIn("cover_big", STAGE)
        self.assertIn("release_cover", STAGE_HEADER)
        self.assertNotIn("catalog_stage_bundle_publish(", LAUNCHER)

    def test_local_source_scanners_accept_cancellation_and_close_handles(self):
        ul = function_body(LAUNCHER, "static int scan_ul(", "static int scan_dir(")
        iso = function_body(LAUNCHER, "static int scan_dir(", "// phase 17:")
        apps = function_body(LAUNCHER, "static int load_games_into(", "static void load_games(void)")
        self.assertIn("src_hdl_cancel_fn cancelled", ul)
        self.assertIn("scan_cancelled(cancelled, cancel_context)", ul)
        self.assertIn("fileXioClose(fd)", ul)
        self.assertIn("ul_part_context part_context", ul)
        self.assertIn("io_error", ul)
        self.assertIn("src_hdl_cancel_fn cancelled", iso)
        self.assertIn("scan_cancelled(cancelled, cancel_context)", iso)
        self.assertIn("(void)fileXioClose(fd)", iso)
        self.assertIn("closedir(d)", iso)
        self.assertIn("src_hdl_scan_cancelable(hdl_add, &c, cancelled, cancel_context)", apps)
        self.assertIn("if (scan_cancelled(cancelled, cancel_context))", apps)
        self.assertIn("if (d && closedir(d) < 0)", apps)
        self.assertIn("cancellable refresh deferred while remote app catalogue is enabled", apps)

    def test_candidate_scan_reports_readiness_without_mutating_active_snapshot_state(self):
        builder = function_body(LAUNCHER, "static int load_games_into(", "static void load_games(void)")
        publish = function_body(LAUNCHER, "static void load_games(void)", "// ---- disc drive")
        self.assertIn("int *snapshot_ready", builder)
        self.assertIn("int *scan_status", builder)
        self.assertNotIn("catalog_active_snapshot_ready =", builder)
        self.assertIn("catalog_active_snapshot_ready = snapshot_ready;", publish)
        self.assertIn("CATALOG_STAGE_SCAN_CANCELLED", builder)

    def test_guard_cancel_callback_fails_closed_on_lock_error(self):
        self.assertIn("catalog_guard_ps2_scan_cancelled", GUARD_PS2)
        self.assertIn("return guard == NULL || catalog_guard_scan_cancelled(guard) != 0;", GUARD_PS2)
        self.assertIn("catalog_guard_ps2_scan_cancelled(void *guard_context)", GUARD_PS2_HEADER)

    def test_network_worker_leases_are_scoped_per_catalog_entry(self):
        art = function_body(LAUNCHER, "static void art_import(void)", "static int download_cover_one")
        art_loop = art[art.index("for (int i = 1; i < ncv && !ra_launching; i++)"):]
        self.assertLess(art.index("background_work_end(&art_busy);"), art.index("for (int i = 1; i < ncv && !ra_launching; i++)"))
        self.assertLess(art_loop.index("background_work_begin(&art_busy)"), art_loop.index("art_import_one(i, &net, max)"))
        self.assertIn("if (!ra_launching && i < ncv) got += art_import_one(i, &net, max);", art_loop)
        self.assertLess(art_loop.index("art_import_one(i, &net, max)"), art_loop.index("background_work_end(&art_busy)"))

        download = function_body(LAUNCHER, "static void download_covers(void)", "// ---- achievements")
        download_loop = download[download.index("for (int i = 1; buf && i < ncv"): ]
        self.assertLess(download.index("background_work_end(&dl_busy);"), download.index("for (int i = 1; buf && i < ncv"))
        self.assertLess(download_loop.index("background_work_begin(&dl_busy)"), download_loop.index("download_cover_one(i, buf, max)"))
        self.assertIn("if (!ra_launching && i < ncv && wants_cover(i))", download_loop)
        self.assertLess(download_loop.index("download_cover_one(i, buf, max)"), download_loop.index("background_work_end(&dl_busy)"))
        self.assertIn("if (!ra_launching) st = https_get", LAUNCHER)
        self.assertIn("Re-read the current generation under each short lease", download)
        self.assertIn("if (!dl_busy && background_work_begin(&dl_busy))", download)
        self.assertNotIn("entry *", download)

        cover = function_body(LAUNCHER, "static void cover_thread(void *arg)", "static int count_dir(")
        inner = cover.index("for (;;) {", cover.index("WaitSema(cover_sema)"))
        begin_cover = cover.index("background_work_begin(&cover_busy)")
        scan_cover = cover.index("for (int i = 1; i < ncv && !ra_launching; i++)")
        load_cover = cover.index("cover_set(selected, im, requested)")
        end_cover = cover.index("background_work_end(&cover_busy)")
        self.assertLess(inner, begin_cover)
        self.assertLess(begin_cover, scan_cover)
        self.assertLess(scan_cover, load_cover)
        self.assertLess(load_cover, end_cover)
        self.assertIn("if (cover_busy || scan_failed || selected < 0)", cover)
        self.assertIn("Reacquire the current generation for each requested cover", cover)

    def test_live_scan_and_apply_are_still_disabled(self):
        self.assertNotIn("catalog_guard_scan_begin(", LAUNCHER)
        self.assertNotIn("catalog_guard_request_apply(", LAUNCHER)


if __name__ == "__main__":
    unittest.main()
