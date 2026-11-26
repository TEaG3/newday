"""Structural regression tests for returning from a failed ELF hand-off."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LauncherHandoffTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.launcher = (ROOT / "launcher.c").read_text(encoding="utf-8")
        cls.gfx = (ROOT / "gfx.c").read_text(encoding="utf-8")
        cls.header = (ROOT / "gfx.h").read_text(encoding="utf-8")
        cls.release_workflow = (ROOT / ".github/workflows/publish-arabic-release.yml").read_text(encoding="utf-8")
        cls.build_workflow = (ROOT / ".github/workflows/build-ps2.yml").read_text(encoding="utf-8")
        cls.lang_ar = (ROOT / "lang_ar.c").read_text(encoding="utf-8")

    def test_low_level_loader_is_called_only_by_recovery_wrapper(self):
        # Every caller must pass through the VBlank recovery path.
        self.assertEqual(len(re.findall(r"\brun_loader\s*\(", self.launcher)), 1)
        self.assertIn("static void run_loader_recover(int argc, char **argv)", self.launcher)

    def test_every_launcher_handoff_uses_recovery_wrapper(self):
        calls = re.findall(r"\brun_loader_recover\s*\(", self.launcher)
        # Seven hand-off sites plus the wrapper definition.
        self.assertEqual(len(calls), 8)
        self.assertIn("if (gfx_resume())", self.launcher)

    def test_resume_reuses_existing_vblank_state(self):
        start = self.gfx.index("int gfx_resume(void)")
        end = self.gfx.index("void gfx_flip(void)", start)
        body = self.gfx[start:end]
        self.assertIn("vsync_sema < 0", body)
        self.assertIn("AddIntcHandler(INTC_VBLANK_S, vsync_handler, 0)", body)
        self.assertIn("EnableIntc(INTC_VBLANK_S)", body)
        self.assertNotIn("gfx_init(", body)

    def test_bios_disc_handoff_also_recovers_display(self):
        self.assertIn('LoadExecPS2(cv[i].disc == D_PS2', self.launcher)
        start = self.launcher.index('LoadExecPS2(cv[i].disc == D_PS2')
        recovery = self.launcher.index('handoff_returned("BIOS");', start)
        return_stmt = self.launcher.index("return;", start)
        self.assertLess(recovery, return_stmt)

    def test_elf_and_bios_returns_share_recovery_function(self):
        self.assertIn('handoff_returned("ELF");', self.launcher)
        start = self.launcher.index("static void handoff_returned(")
        end = self.launcher.index("static void run_loader_recover(", start)
        body = self.launcher[start:end]
        self.assertIn("gfx_resume()", body)
        self.assertIn('L("Could not start")', body)


    def test_background_workers_are_quiesced_before_iop_reset(self):
        start = self.launcher.index("static int background_quiesce(void)")
        end = self.launcher.index("static void background_resume_if_idle(void)", start)
        body = self.launcher[start:end]
        self.assertIn("WaitSema(cover_lock)", body)
        self.assertIn("ra_launching = 1", body)
        self.assertIn("SignalSema(cover_lock)", body)
        self.assertIn("ra_abort = 1", body)
        self.assertIn("ra_busy", body)
        self.assertIn("art_busy", body)
        self.assertIn("update_busy", body)
        self.assertIn("dl_busy", body)
        self.assertIn("dl_state == 1 || dl_state == 2", body)
        self.assertIn("cover_busy", body)
        self.assertIn("icon_busy", body)
        self.assertIn("disc_busy", body)
        self.assertIn("return !cover_busy && !icon_busy && !disc_busy", body)

        launch_start = self.launcher.index("static void launch(int i)")
        self.assertLess(
            self.launcher.index("background_quiesce()", launch_start),
            self.launcher.index("run_loader_recover", launch_start),
        )
        restart_start = self.launcher.index("static int restart_self(void)")
        self.assertLess(
            self.launcher.index("background_quiesce()", restart_start),
            self.launcher.index("run_loader_recover", restart_start),
        )

    def test_cover_worker_stops_starting_reads_after_launch_gate_closes(self):
        start = self.launcher.index("static void cover_thread(void *arg)")
        end = self.launcher.index("static int count_dir(", start)
        body = self.launcher[start:end]
        self.assertIn("background_work_begin(&cover_busy)", body)
        self.assertIn("i < ncv && !ra_launching", body)
        self.assertIn("background_work_end(&cover_busy)", body)

    def test_icon_worker_is_quiesced_before_iop_reset(self):
        start = self.launcher.index("static void icon_thread(void *arg)")
        end = self.launcher.index("// ---- covers ----", start)
        body = self.launcher[start:end]
        self.assertIn("background_work_begin(&icon_busy)", body)
        self.assertIn("background_work_end(&icon_busy)", body)
        self.assertIn("SignalSema(icon_sema)", body)
        self.assertIn("usleep(10000)", body)

        work_start = body.index("if (stub_install == 1)")
        work_end = body.index("background_work_end(&icon_busy)", work_start)
        work = body[work_start:work_end]
        for operation in ("install_stub()", "wiz_job();", 'fopen(STATE_INI, "w")',
                          "vmc_load(i)", "newest_save(i, &port)", "load_icon(i)"):
            self.assertIn(operation, work)
        self.assertNotIn("continue;", work)

        loader_start = self.launcher.index("static void loader(void *arg)")
        lock = self.launcher.index("cover_lock = CreateSema", loader_start)
        worker = self.launcher.index("ee_thread_t it = { .func = icon_thread", loader_start)
        self.assertLess(lock, worker)
        self.assertIn("cover_lock >= 0 && icon_sema >= 0 ? CreateThread(&it)", self.launcher[worker:])

    def test_disc_worker_stops_libcdvd_and_cover_io_after_gate_closes(self):
        start = self.launcher.index("static void disc_thread(void *arg)")
        end = self.launcher.index("// ---- config", start)
        body = self.launcher[start:end]
        begin = body.index("background_work_begin(&disc_busy)")
        end_work = body.rindex("background_work_end(&disc_busy)")
        work = body[begin:end_work]
        for operation in ("disc_state(sceCdGetDiskType())", "sceCdDiskReady(0)",
                          "disc_cnf(cnf, sizeof(cnf))", "cv[i].serial",
                          "cv[i].title", "load_covers(d)"):
            self.assertIn(operation, work)
        self.assertLess(begin, body.index("disc_state(sceCdGetDiskType())"))
        self.assertIn("sleep(1);", body)
        skip = body.index("if ((st == last")
        release = body.index("background_work_end(&disc_busy)", skip)
        continue_stmt = body.index("continue;", skip)
        self.assertLess(release, continue_stmt)

        loader_start = self.launcher.index("static void loader(void *arg)")
        lock = self.launcher.index("cover_lock = CreateSema", loader_start)
        worker = self.launcher.index("ee_thread_t dt = { .func = disc_thread", loader_start)
        self.assertLess(lock, worker)

    def test_background_gate_reopens_only_after_tracked_workers_are_idle(self):
        start = self.launcher.index("static void background_resume_if_idle(void)")
        end = self.launcher.index("static void ra_log(", start)
        body = self.launcher[start:end]
        self.assertIn("ra_busy", body)
        self.assertIn("art_busy", body)
        self.assertIn("update_busy", body)
        self.assertIn("dl_busy", body)
        self.assertIn("dl_state != 1 && dl_state != 2", body)
        self.assertIn("cover_busy", body)
        self.assertIn("icon_busy", body)
        self.assertIn("disc_busy", body)
        self.assertIn("ra_abort = 0", body)
        self.assertIn("Background work is still active; try again", self.launcher)
        self.assertIn(
            '{"Background work is still active; try again", "العمل في الخلفية لم ينته بعد، حاول مرة أخرى"}',
            self.lang_ar,
        )


    def test_every_cover_and_network_worker_uses_the_shared_gate(self):
        ranges = (
            ("static void art_import(void)", "static void download_covers(void)"),
            ("static void download_covers(void)", "// ---- achievements"),
            ("static void update_check(void)", "static u8 loader_stack"),
            ("static void cover_thread(void *arg)", "static int count_dir("),
        )
        for start_marker, end_marker in ranges:
            start = self.launcher.index(start_marker)
            end = self.launcher.index(end_marker, start)
            body = self.launcher[start:end]
            self.assertIn("background_work_begin(&", body)
            self.assertIn("background_work_end(&", body)

        worker_start = self.launcher.index("while (ra_sema >= 0)")
        worker_end = self.launcher.index("ExitThread();", worker_start)
        self.assertIn("background_work_begin(&ra_busy)", self.launcher[worker_start:worker_end])

    def test_long_background_requests_stop_between_items_after_gate_closes(self):
        art_start = self.launcher.index("static void art_import(void)")
        art_end = self.launcher.index("static void download_covers(void)", art_start)
        art = self.launcher[art_start:art_end]
        self.assertIn("i < ncv && !ra_launching", art)
        self.assertIn("st != 200 && !ra_launching", art)

        download_start = self.launcher.index("static void download_covers(void)")
        download_end = self.launcher.index("// ---- achievements", download_start)
        self.assertIn("dl_state == 2 && !ra_launching", self.launcher[download_start:download_end])
        self.assertIn("NET_ERR_CONNECT && !dl_done && !ra_launching", self.launcher[download_start:download_end])

        update_start = self.launcher.index("static void update_check(void)")
        update_end = self.launcher.index("static u8 loader_stack", update_start)
        self.assertIn("st == 200 && !ra_launching", self.launcher[update_start:update_end])


    def test_shared_worker_gate_serializes_start_against_quiesce(self):
        start = self.launcher.index("static int background_work_begin(volatile int *busy)\n{")
        end = self.launcher.index("static void background_work_end(", start)
        body = self.launcher[start:end]
        self.assertIn("WaitSema(cover_lock)", body)
        self.assertIn("!ra_launching", body)
        self.assertIn("*busy = 1", body)
        self.assertIn("SignalSema(cover_lock)", body)

        download_start = self.launcher.index("static void download_covers(void)")
        download_end = self.launcher.index("// ---- achievements", download_start)
        download = self.launcher[download_start:download_end]
        self.assertLess(
            download.index("background_work_begin(&dl_busy)"),
            download.index("wants_cover(i)"),
        )

    def test_resume_is_declared_in_public_header(self):
        self.assertIn("int gfx_resume(void);", self.header)

    def test_release_workflow_uses_the_recovery_wrapper(self):
        self.assertIn("run_loader_recover(8, opl_argv)", self.release_workflow)
        self.assertNotIn("run_loader(8, opl_argv)", self.release_workflow)
        self.assertIn("USBExtreme UL يستخدم OPL فقط", self.release_workflow)

    def test_release_workflow_uses_the_gitlink_pinned_teag3_opl_source(self):
        self.assertIn("repository: TEaG3/OPL", self.release_workflow)
        self.assertIn("git ls-tree HEAD engine/opl", self.release_workflow)
        self.assertIn("ref: ${{ steps.opl-ref.outputs.sha }}", self.release_workflow)
        self.assertIn("submodules: recursive", self.build_workflow)
        self.assertNotIn("repository: ps2homebrew/Open-PS2-Loader", self.release_workflow)

    def test_both_opl_builds_generate_and_verify_complete_arabic(self):
        for workflow in (self.build_workflow, self.release_workflow):
            self.assertIn("check_arabic_translation.py", workflow)
            self.assertIn("make languages", workflow)
            self.assertIn("test -s lng/lang_Arabic.lng", workflow)
            self.assertIn("grep -Fq 'العودة' lng/lang_Arabic.lng", workflow)
            self.assertIn("test -s lng_src/thirdparty/font_Arabic.ttf", workflow)

    def test_lwnbd_script_is_executable_before_make_clean(self):
        # OPL's clean target executes the downloaded helper directly, not through bash.
        for workflow in (self.build_workflow, self.release_workflow):
            download = workflow.index("download_lwNBD.sh -o download_lwNBD.sh")
            chmod = workflow.index("chmod +x download_lwNBD.sh", download)
            executable = workflow.index("test -x download_lwNBD.sh", chmod)
            clean = workflow.index("make clean", executable)
            self.assertLess(download, chmod)
            self.assertLess(chmod, executable)
            self.assertLess(executable, clean)

    def test_release_opl_build_flags_match_ci(self):
        flags = "make NOT_PACKED=1 RTL=1 PADEMU=1"
        self.assertIn(flags, self.build_workflow)
        self.assertIn(flags, self.release_workflow)
        self.assertIn("test -s include/lang_autogen.h", self.release_workflow)
        self.assertIn("test -s src/lang_internal.c", self.release_workflow)


if __name__ == "__main__":
    unittest.main()
