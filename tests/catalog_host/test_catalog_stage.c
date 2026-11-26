#include "catalog_stage.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static catalog_stage_snapshot old_snapshot;
static catalog_stage_snapshot new_snapshot;

static void set_record(catalog_stage_record *record, uint8_t source,
                       uint8_t unit, uint8_t kind, const char *path,
                       uint64_t size, uint32_t fingerprint)
{
    memset(record, 0, sizeof(*record));
    record->source_type = source;
    record->source_unit = unit;
    record->kind = kind;
    record->size = size;
    record->fingerprint = fingerprint;
    snprintf(record->path, sizeof(record->path), "%s", path);
}

static void test_path_and_validation(void)
{
    catalog_stage_snapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    assert(catalog_stage_path_is_safe("DVD/Game 1.iso"));
    assert(catalog_stage_path_is_safe("CD/Disc Two.zso"));
    assert(!catalog_stage_path_is_safe(""));
    assert(!catalog_stage_path_is_safe("../outside.iso"));
    assert(!catalog_stage_path_is_safe("DVD/../outside.iso"));
    assert(!catalog_stage_path_is_safe("DVD//game.iso"));
    assert(!catalog_stage_path_is_safe("DVD/game.iso/"));
    assert(!catalog_stage_path_is_safe("/DVD/game.iso"));
    assert(!catalog_stage_path_is_safe("DVD\\game.iso"));
    assert(!catalog_stage_path_is_safe("DVD/C:game.iso"));

    snapshot.count = 2;
    set_record(&snapshot.records[0], 1, 0, 0, "DVD/Game.iso", 100, 1);
    set_record(&snapshot.records[1], 1, 0, 0, "DVD/Game.iso", 100, 1);
    assert(catalog_stage_snapshot_validate(&snapshot) == CATALOG_STAGE_ERR_DUPLICATE);
    snapshot.records[1].path[4] = 'X';
    assert(catalog_stage_snapshot_validate(&snapshot) == CATALOG_STAGE_OK);
    snapshot.count = CATALOG_STAGE_MAX_ITEMS + 1u;
    assert(catalog_stage_snapshot_validate(&snapshot) == CATALOG_STAGE_ERR_CAPACITY);
    snapshot.count = 1;
    memset(snapshot.records[0].path, 'A', sizeof(snapshot.records[0].path));
    assert(catalog_stage_snapshot_find(&snapshot, 1, 0, 0, "DVD/Game.iso") == -1);
}

static void test_snapshot_append(void)
{
    catalog_stage_snapshot snapshot;
    catalog_stage_record record;

    memset(&snapshot, 0, sizeof(snapshot));
    set_record(&record, 1, 0, 0, "DVD/First.iso", 100, 11);
    assert(catalog_stage_snapshot_append(&snapshot, &record) == CATALOG_STAGE_OK);
    assert(snapshot.count == 1);
    assert(strcmp(snapshot.records[0].path, "DVD/First.iso") == 0);

    set_record(&record, 1, 0, 0, "../escape.iso", 100, 11);
    assert(catalog_stage_snapshot_append(&snapshot, &record) == CATALOG_STAGE_ERR_PATH);
    assert(snapshot.count == 1); /* invalid records never partially append */

    set_record(&record, 1, 0, 0, "DVD/First.iso", 200, 22);
    assert(catalog_stage_snapshot_append(&snapshot, &record) == CATALOG_STAGE_OK);
    assert(snapshot.count == 2);
    assert(catalog_stage_snapshot_validate(&snapshot) == CATALOG_STAGE_ERR_DUPLICATE);

    snapshot.count = CATALOG_STAGE_MAX_ITEMS;
    set_record(&record, 1, 0, 0, "DVD/Overflow.iso", 1, 1);
    assert(catalog_stage_snapshot_append(&snapshot, &record) == CATALOG_STAGE_ERR_CAPACITY);
    assert(snapshot.count == CATALOG_STAGE_MAX_ITEMS);

    snapshot.count = 0;
    memset(record.path, 'X', sizeof(record.path)); /* missing NUL is rejected before copying */
    assert(catalog_stage_snapshot_append(&snapshot, &record) == CATALOG_STAGE_ERR_PATH);
    assert(snapshot.count == 0);
}

static void test_snapshot_index_map(void)
{
    catalog_stage_snapshot old_snapshot;
    catalog_stage_snapshot new_snapshot;
    int16_t old_to_new[3] = {77, 77, 77};

    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    old_snapshot.count = 3;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&old_snapshot.records[1], 8, 0, 0, "DVD/Beta.iso", 200, 22);
    set_record(&old_snapshot.records[2], 2, 0, 0, "HDL/Gone", 300, 33);

    new_snapshot.count = 3;
    set_record(&new_snapshot.records[0], 8, 0, 0, "DVD/Beta.iso", 201, 23); /* changed but same identity */
    set_record(&new_snapshot.records[1], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&new_snapshot.records[2], 1, 0, 0, "DVD/Gamma.iso", 400, 44);

    assert(catalog_stage_snapshot_map(&old_snapshot, &new_snapshot, old_to_new, 3) == CATALOG_STAGE_OK);
    assert(old_to_new[0] == 1);
    assert(old_to_new[1] == 0);
    assert(old_to_new[2] == -1);

    old_to_new[0] = old_to_new[1] = old_to_new[2] = 77;
    assert(catalog_stage_snapshot_map(&old_snapshot, &new_snapshot, old_to_new, 2) == CATALOG_STAGE_ERR_CAPACITY);
    assert(old_to_new[0] == 77 && old_to_new[1] == 77 && old_to_new[2] == 77);

    set_record(&new_snapshot.records[2], 1, 0, 0, "DVD/Alpha.iso", 999, 99);
    assert(catalog_stage_snapshot_map(&old_snapshot, &new_snapshot, old_to_new, 3) == CATALOG_STAGE_ERR_DUPLICATE);
    assert(old_to_new[0] == 77 && old_to_new[1] == 77 && old_to_new[2] == 77);
}

typedef struct {
    unsigned cover_token;
    unsigned vmc_token;
    unsigned achievement_token;
} test_sidecar_state;

static void test_sidecar_migration(void)
{
    catalog_stage_snapshot old_snapshot;
    catalog_stage_snapshot new_snapshot;
    test_sidecar_state old_state[3] = {{11, 12, 13}, {21, 22, 23}, {31, 32, 33}};
    test_sidecar_state new_state[3] = {{99, 99, 99}, {99, 99, 99}, {99, 99, 99}};

    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    old_snapshot.count = 3;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&old_snapshot.records[1], 8, 0, 0, "DVD/Beta.iso", 200, 22);
    set_record(&old_snapshot.records[2], 2, 0, 0, "HDL/Removed", 300, 33);

    new_snapshot.count = 3;
    set_record(&new_snapshot.records[0], 8, 0, 0, "DVD/Beta.iso", 201, 23); /* changed: reset */
    set_record(&new_snapshot.records[1], 1, 0, 0, "DVD/Alpha.iso", 100, 11); /* unchanged: preserve */
    set_record(&new_snapshot.records[2], 1, 0, 0, "DVD/Added.iso", 400, 44); /* new: zero */

    assert(catalog_stage_sidecar_migrate(&old_snapshot, &new_snapshot,
                                         old_state, 3, new_state, 3,
                                         sizeof(test_sidecar_state)) == CATALOG_STAGE_OK);
    assert(new_state[0].cover_token == 0 && new_state[0].vmc_token == 0 && new_state[0].achievement_token == 0);
    assert(new_state[1].cover_token == 11 && new_state[1].vmc_token == 12 && new_state[1].achievement_token == 13);
    assert(new_state[2].cover_token == 0 && new_state[2].vmc_token == 0 && new_state[2].achievement_token == 0);

    new_state[0].cover_token = new_state[1].cover_token = new_state[2].cover_token = 77;
    assert(catalog_stage_sidecar_migrate(&old_snapshot, &new_snapshot,
                                         old_state, 2, new_state, 3,
                                         sizeof(test_sidecar_state)) == CATALOG_STAGE_ERR_CAPACITY);
    assert(new_state[0].cover_token == 77 && new_state[1].cover_token == 77 && new_state[2].cover_token == 77);

    {
        unsigned char overlapping[sizeof(test_sidecar_state) * 3 + 1];
        memset(overlapping, 0x5a, sizeof(overlapping));
        assert(catalog_stage_sidecar_migrate(&old_snapshot, &new_snapshot,
                                             overlapping, 3, overlapping + 1, 3,
                                             sizeof(test_sidecar_state)) == CATALOG_STAGE_ERR_ARGUMENT);
        for (size_t i = 0; i < sizeof(overlapping); i++) assert(overlapping[i] == 0x5a);
    }

    set_record(&new_snapshot.records[2], 1, 0, 0, "DVD/Alpha.iso", 999, 99);
    assert(catalog_stage_sidecar_migrate(&old_snapshot, &new_snapshot,
                                         old_state, 3, new_state, 3,
                                         sizeof(test_sidecar_state)) == CATALOG_STAGE_ERR_DUPLICATE);
    assert(new_state[0].cover_token == 77 && new_state[1].cover_token == 77 && new_state[2].cover_token == 77);
}

static void test_diff_detects_large_file_size_changes(void)
{
    catalog_stage_snapshot old_snapshot;
    catalog_stage_snapshot new_snapshot;
    catalog_stage_diff diff;

    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    old_snapshot.count = 1;
    new_snapshot.count = 1;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Large.iso",
               UINT64_C(0x100000000), 7);
    set_record(&new_snapshot.records[0], 1, 0, 0, "DVD/Large.iso",
               UINT64_C(0x100000001), 7);

    memset(&diff, 0, sizeof(diff));
    assert(catalog_stage_snapshot_diff(&old_snapshot, &new_snapshot, &diff) == CATALOG_STAGE_OK);
    assert(diff.added_count == 0 && diff.removed_count == 0);
    assert(diff.changed_count == 1);
    assert(diff.changed_old[0] == 0 && diff.changed_new[0] == 0);
}

static void test_diff_and_stable_selection(void)
{
    catalog_stage_diff diff;
    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));

    old_snapshot.count = 3;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&old_snapshot.records[1], 8, 0, 0, "DVD/Beta.iso", 200, 22);
    set_record(&old_snapshot.records[2], 2, 0, 0, "HDL/SLUS_12345", 300, 33);

    new_snapshot.count = 3;
    set_record(&new_snapshot.records[0], 8, 0, 0, "DVD/Beta.iso", 201, 23);
    set_record(&new_snapshot.records[1], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&new_snapshot.records[2], 1, 0, 0, "DVD/Gamma.iso", 400, 44);

    memset(&diff, 0, sizeof(diff));
    assert(catalog_stage_snapshot_diff(&old_snapshot, &new_snapshot, &diff) == CATALOG_STAGE_OK);
    assert(diff.added_count == 1 && diff.added[0] == 2);
    assert(diff.removed_count == 1 && diff.removed[0] == 2);
    assert(diff.changed_count == 1 && diff.changed_old[0] == 1 && diff.changed_new[0] == 0);
    assert(catalog_stage_snapshot_find(&new_snapshot, 1, 0, 0, "DVD/Alpha.iso") == 1);
    assert(catalog_stage_snapshot_find(&new_snapshot, 1, 0, 0, "DVD/Gamma.iso") == 2);

    diff.added_count = 77;
    new_snapshot.records[2] = new_snapshot.records[1];
    assert(catalog_stage_snapshot_diff(&old_snapshot, &new_snapshot, &diff) == CATALOG_STAGE_ERR_DUPLICATE);
    assert(diff.added_count == 77);
}

typedef struct {
    pthread_mutex_t mutex;
} mutex_context;

static int guard_lock(void *context)
{
    mutex_context *m = (mutex_context *)context;
    return pthread_mutex_lock(&m->mutex) == 0;
}

static int guard_unlock(void *context)
{
    mutex_context *m = (mutex_context *)context;
    return pthread_mutex_unlock(&m->mutex) == 0;
}

typedef struct {
    catalog_guard *guard;
    pthread_mutex_t sync_mutex;
    pthread_cond_t sync_cond;
    int acquired;
    int release;
} reader_thread_context;

static void *reader_thread(void *arg)
{
    reader_thread_context *ctx = (reader_thread_context *)arg;
    assert(catalog_guard_read_begin(ctx->guard));
    assert(pthread_mutex_lock(&ctx->sync_mutex) == 0);
    ctx->acquired = 1;
    assert(pthread_cond_broadcast(&ctx->sync_cond) == 0);
    while (!ctx->release)
        assert(pthread_cond_wait(&ctx->sync_cond, &ctx->sync_mutex) == 0);
    assert(pthread_mutex_unlock(&ctx->sync_mutex) == 0);
    assert(catalog_guard_read_end(ctx->guard));
    return NULL;
}


static void test_download_gate(void)
{
    catalog_guard guard;
    mutex_context mutex;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));

    /* Storage I/O must block new scans and downloads before either can start. */
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(!catalog_guard_can_scan(&guard));
    assert(!catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_can_download(&guard));
    assert(!catalog_guard_download_begin(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    assert(catalog_guard_can_download(&guard));
    assert(catalog_guard_download_begin(&guard));
    assert(!catalog_guard_can_download(&guard));
    assert(!catalog_guard_read_begin(&guard));
    assert(!catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_can_scan(&guard));
    assert(!catalog_guard_request_apply(&guard));
    assert(!catalog_guard_try_apply(&guard));

    /* Entering gameplay cancels an active download; the hand-off waits for it to end. */
    assert(!catalog_guard_set_activity(&guard, 1, 0));
    assert(catalog_guard_download_cancelled(&guard));
    assert(!catalog_guard_can_download(&guard));
    assert(!catalog_guard_download_begin(&guard));
    assert(catalog_guard_download_end(&guard));
    assert(!catalog_guard_download_cancelled(&guard));
    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(catalog_guard_set_activity(&guard, 0, 0));
    assert(catalog_guard_can_download(&guard));

    /* A download cannot overlap a scan, a reader, or a pending/applying snapshot.
       Read-only UI leases may continue while the inactive candidate is being built. */
    assert(catalog_guard_scan_begin(&guard));
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(!catalog_guard_download_begin(&guard));
    assert(!catalog_guard_set_activity(&guard, 0, 1)); /* new I/O cancels and waits for the scan */
    assert(catalog_guard_scan_cancelled(&guard));
    assert(!catalog_guard_scan_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    /* Storage I/O cancels an active staged scan and waits until its candidate is discarded. */
    assert(catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_set_activity(&guard, 0, 1));
    assert(catalog_guard_scan_cancelled(&guard));
    assert(!catalog_guard_scan_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    assert(catalog_guard_read_begin(&guard));
    assert(!catalog_guard_download_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_request_apply(&guard));
    assert(!catalog_guard_download_begin(&guard));
    assert(catalog_guard_try_apply(&guard));
    assert(!catalog_guard_can_download(&guard));
    assert(catalog_guard_finish_apply(&guard));

    assert(catalog_guard_download_begin(&guard));
    assert(!catalog_guard_set_activity(&guard, 0, 1)); /* storage I/O waits for the cancelled download */
    assert(catalog_guard_download_cancelled(&guard));
    assert(catalog_guard_download_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(!catalog_guard_download_begin(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 0));
    assert(catalog_guard_can_download(&guard));
    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}

static void test_guard_and_concurrent_reader(void)
{
    catalog_guard guard;
    mutex_context mutex;
    reader_thread_context reader;
    pthread_t thread;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));
    assert(catalog_guard_can_scan(&guard));
    assert(catalog_guard_read_begin(&guard));
    assert(!catalog_guard_can_scan(&guard));
    assert(!catalog_guard_scan_begin(&guard)); /* a scan cannot start while a reader holds the active snapshot */
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_can_scan(&guard));
    assert(catalog_guard_read_begin(&guard)); /* render may keep reading the active snapshot */
    assert(catalog_guard_read_end(&guard));
    assert(!catalog_guard_set_activity(&guard, 1, 0)); /* hand-off cancels and drains the candidate scan */
    assert(catalog_guard_scan_cancelled(&guard));
    assert(!catalog_guard_request_apply(&guard)); /* no candidate may be published while scanning */
    assert(!catalog_guard_try_apply(&guard));
    assert(!catalog_guard_scan_end(&guard)); /* a cancelled scan cannot be published */
    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(catalog_guard_set_activity(&guard, 0, 0));
    assert(catalog_guard_can_scan(&guard));

    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(!catalog_guard_scan_begin(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 0));
    assert(catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_set_activity(&guard, 0, 1)); /* storage I/O cancels and waits for the active scan */
    assert(catalog_guard_scan_cancelled(&guard));
    assert(!catalog_guard_scan_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(catalog_guard_set_activity(&guard, 0, 0));
    assert(catalog_guard_scan_begin(&guard));
    assert(catalog_guard_scan_end(&guard));

    /* An apply request must not close the reader gate during launch or storage I/O. */
    assert(catalog_guard_set_activity(&guard, 1, 1));
    assert(!catalog_guard_request_apply(&guard));
    assert(!catalog_guard_can_scan(&guard));
    assert(!catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_try_apply(&guard));
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    /* An I/O-only busy interval must also reject apply requests without closing readers. */
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(!catalog_guard_request_apply(&guard));
    assert(!catalog_guard_try_apply(&guard));
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    /* If gameplay starts after a request, cancel the pending apply and reopen readers. */
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(!catalog_guard_try_apply(&guard));
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    /* Storage I/O starting after a request must cancel it for the same reason. */
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(!catalog_guard_try_apply(&guard));
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    assert(catalog_guard_request_apply(&guard));
    assert(catalog_guard_try_apply(&guard));
    assert(catalog_guard_finish_apply(&guard));

    memset(&reader, 0, sizeof(reader));
    reader.guard = &guard;
    assert(pthread_mutex_init(&reader.sync_mutex, NULL) == 0);
    assert(pthread_cond_init(&reader.sync_cond, NULL) == 0);
    assert(pthread_create(&thread, NULL, reader_thread, &reader) == 0);

    assert(pthread_mutex_lock(&reader.sync_mutex) == 0);
    while (!reader.acquired)
        assert(pthread_cond_wait(&reader.sync_cond, &reader.sync_mutex) == 0);
    assert(pthread_mutex_unlock(&reader.sync_mutex) == 0);

    assert(catalog_guard_request_apply(&guard));
    assert(!catalog_guard_read_begin(&guard));
    assert(!catalog_guard_try_apply(&guard));

    assert(pthread_mutex_lock(&reader.sync_mutex) == 0);
    reader.release = 1;
    assert(pthread_cond_broadcast(&reader.sync_cond) == 0);
    assert(pthread_mutex_unlock(&reader.sync_mutex) == 0);
    assert(pthread_join(thread, NULL) == 0);

    assert(catalog_guard_try_apply(&guard));
    assert(!catalog_guard_set_activity(&guard, 1, 0));
    assert(catalog_guard_finish_apply(&guard));
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_cancel_apply(&guard));

    assert(pthread_cond_destroy(&reader.sync_cond) == 0);
    assert(pthread_mutex_destroy(&reader.sync_mutex) == 0);
    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}

typedef struct {
    catalog_guard *guard;
    catalog_stage_record records[3];
    size_t count;
    size_t index;
    int cancel_at;
    int error_at;
    int generate;
    size_t generated_count;
} scan_fixture;

static int scan_fixture_next(void *opaque, catalog_stage_record *record)
{
    scan_fixture *fixture = (scan_fixture *)opaque;
    if (fixture->cancel_at >= 0 && (int)fixture->index == fixture->cancel_at) {
        assert(!catalog_guard_set_activity(fixture->guard, 0, 1));
        fixture->cancel_at = -1;
    }
    if (fixture->error_at >= 0 && (int)fixture->index == fixture->error_at)
        return -1;
    if (fixture->generate) {
        if (fixture->index >= fixture->generated_count)
            return 0;
        memset(record, 0, sizeof(*record));
        record->source_type = 1;
        record->kind = 1;
        snprintf(record->path, sizeof(record->path), "DVD/Game%04u.iso",
                 (unsigned)fixture->index);
        record->size = (uint32_t)fixture->index;
        record->fingerprint = (uint32_t)fixture->index + 1u;
        ++fixture->index;
        return 1;
    }
    if (fixture->index >= fixture->count)
        return 0;
    *record = fixture->records[fixture->index++];
    return 1;
}

static void test_staged_scan_builder(void)
{
    mutex_context mutex;
    catalog_guard guard;
    catalog_stage_snapshot candidate;
    scan_fixture fixture;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));

    memset(&candidate, 0, sizeof(candidate));
    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = -1;
    fixture.error_at = -1;
    fixture.count = 2;
    set_record(&fixture.records[0], 1, 0, 1, "DVD/First.iso", 10, 11);
    set_record(&fixture.records[1], 1, 0, 1, "DVD/Second.zso", 20, 22);
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_OK);
    assert(candidate.count == 2);
    assert(catalog_stage_snapshot_validate(&candidate) == CATALOG_STAGE_OK);
    assert(!guard.scanning);

    /* A reader prevents scan start; no partial candidate is exposed. */
    assert(catalog_guard_read_begin(&guard));
    candidate.count = 9;
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_ERR_BUSY);
    assert(candidate.count == 0);
    assert(catalog_guard_read_end(&guard));

    /* New storage I/O cancels the scan and must wait until the candidate is discarded. */
    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = 1;
    fixture.error_at = -1;
    fixture.count = 2;
    set_record(&fixture.records[0], 1, 0, 1, "DVD/First.iso", 10, 11);
    set_record(&fixture.records[1], 1, 0, 1, "DVD/Second.iso", 20, 22);
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_CANCELLED);
    assert(candidate.count == 0);
    assert(!guard.scanning);
    assert(catalog_guard_set_activity(&guard, 0, 1));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    /* Source errors and invalid/duplicate records never leave a partial candidate. */
    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = -1;
    fixture.error_at = 1;
    fixture.count = 2;
    set_record(&fixture.records[0], 1, 0, 1, "DVD/First.iso", 10, 11);
    set_record(&fixture.records[1], 1, 0, 1, "DVD/Second.iso", 20, 22);
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_ERR_SOURCE);
    assert(candidate.count == 0);

    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = -1;
    fixture.error_at = -1;
    fixture.count = 1;
    set_record(&fixture.records[0], 1, 0, 1, "../escape.iso", 10, 11);
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_ERR_RECORD);
    assert(candidate.count == 0);

    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = -1;
    fixture.error_at = -1;
    fixture.count = 1;
    memset(fixture.records[0].path, 'X', sizeof(fixture.records[0].path)); /* no terminator */
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_ERR_RECORD);
    assert(candidate.count == 0);

    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = -1;
    fixture.error_at = -1;
    fixture.count = 2;
    set_record(&fixture.records[0], 1, 0, 1, "DVD/Duplicate.iso", 10, 11);
    set_record(&fixture.records[1], 1, 0, 1, "DVD/Duplicate.iso", 20, 22);
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_ERR_RECORD);
    assert(candidate.count == 0);
    assert(!guard.scanning);

    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = -1;
    fixture.error_at = -1;
    fixture.generate = 1;
    fixture.generated_count = CATALOG_STAGE_MAX_ITEMS + 1u;
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_ERR_CAPACITY);
    assert(candidate.count == 0);
    assert(!guard.scanning);

    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}


typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int entered;
    int done;
    int release;
    int handle_open;
    int handle_closed;
    int next_calls;
} concurrent_scan_fixture;

typedef struct {
    catalog_guard *guard;
    concurrent_scan_fixture *fixture;
    catalog_stage_snapshot *candidate;
    int result;
} concurrent_scan_thread_context;

static int concurrent_scan_next(void *opaque, catalog_stage_record *record)
{
    concurrent_scan_fixture *fixture = (concurrent_scan_fixture *)opaque;

    assert(pthread_mutex_lock(&fixture->mutex) == 0);
    ++fixture->next_calls;
    if (fixture->next_calls > 1) {
        assert(pthread_mutex_unlock(&fixture->mutex) == 0);
        return 0;
    }

    /* Model one source handle that stays open while the read is in progress. */
    fixture->handle_open = 1;
    fixture->entered = 1;
    assert(pthread_cond_broadcast(&fixture->cond) == 0);
    while (!fixture->release)
        assert(pthread_cond_wait(&fixture->cond, &fixture->mutex) == 0);
    fixture->handle_open = 0;
    fixture->handle_closed = 1;
    assert(pthread_mutex_unlock(&fixture->mutex) == 0);

    set_record(record, 1, 0, 1, "DVD/Concurrent.iso", 42, 7);
    return 1;
}

static void *concurrent_scan_worker(void *opaque)
{
    concurrent_scan_thread_context *context = (concurrent_scan_thread_context *)opaque;
    context->result = catalog_stage_scan(context->guard, concurrent_scan_next,
                                         context->fixture, context->candidate);
    assert(pthread_mutex_lock(&context->fixture->mutex) == 0);
    context->fixture->done = 1;
    assert(pthread_cond_broadcast(&context->fixture->cond) == 0);
    assert(pthread_mutex_unlock(&context->fixture->mutex) == 0);
    return NULL;
}

static void test_concurrent_scan_cancellation(void)
{
    catalog_guard guard;
    mutex_context mutex;
    concurrent_scan_fixture fixture;
    concurrent_scan_thread_context worker;
    catalog_stage_snapshot candidate;
    pthread_t thread;

    memset(&fixture, 0, sizeof(fixture));
    memset(&candidate, 0, sizeof(candidate));
    memset(&worker, 0, sizeof(worker));

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));
    assert(pthread_mutex_init(&fixture.mutex, NULL) == 0);
    assert(pthread_cond_init(&fixture.cond, NULL) == 0);

    worker.guard = &guard;
    worker.fixture = &fixture;
    worker.candidate = &candidate;
    worker.result = CATALOG_STAGE_SCAN_ERR_ARGUMENT;
    assert(pthread_create(&thread, NULL, concurrent_scan_worker, &worker) == 0);

    /* Wait until the source callback has opened its handle and is blocked in I/O. */
    assert(pthread_mutex_lock(&fixture.mutex) == 0);
    while (!fixture.entered && !fixture.done)
        assert(pthread_cond_wait(&fixture.cond, &fixture.mutex) == 0);
    assert(fixture.entered && fixture.handle_open && !fixture.handle_closed);
    assert(pthread_mutex_unlock(&fixture.mutex) == 0);

    /* Gameplay cancels the scan, but must wait for the callback to close its handle. */
    assert(!catalog_guard_set_activity(&guard, 1, 0));
    assert(catalog_guard_scan_cancelled(&guard) == 1);

    assert(pthread_mutex_lock(&fixture.mutex) == 0);
    fixture.release = 1;
    assert(pthread_cond_broadcast(&fixture.cond) == 0);
    assert(pthread_mutex_unlock(&fixture.mutex) == 0);
    assert(pthread_join(thread, NULL) == 0);

    assert(worker.result == CATALOG_STAGE_SCAN_CANCELLED);
    assert(candidate.count == 0); /* a cancelled source never exposes a partial candidate */
    assert(pthread_mutex_lock(&fixture.mutex) == 0);
    assert(fixture.handle_closed && !fixture.handle_open);
    assert(fixture.next_calls == 1); /* no further source reads after cancellation */
    assert(pthread_mutex_unlock(&fixture.mutex) == 0);
    assert(!guard.scanning);

    /* The caller may proceed with gameplay once the cancelled callback has drained. */
    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    assert(pthread_cond_destroy(&fixture.cond) == 0);
    assert(pthread_mutex_destroy(&fixture.mutex) == 0);
    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}

static void test_safe_snapshot_publish(void)
{
    catalog_guard guard;
    mutex_context mutex;
    catalog_stage_snapshot active_snapshot, candidate_snapshot;
    catalog_stage_snapshot *active = &active_snapshot;
    catalog_stage_snapshot *candidate = &candidate_snapshot;

    memset(&active_snapshot, 0, sizeof(active_snapshot));
    memset(&candidate_snapshot, 0, sizeof(candidate_snapshot));
    set_record(&active_snapshot.records[0], 1, 0, 1, "DVD/Old.iso", 100, 1);
    active_snapshot.count = 1;
    set_record(&candidate_snapshot.records[0], 1, 0, 1, "DVD/New.iso", 200, 2);
    candidate_snapshot.count = 1;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));

    /* Publishing requires an explicit request and never changes pointers while busy. */
    assert(catalog_stage_snapshot_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active == &active_snapshot && candidate == &candidate_snapshot);

    /* A pending apply blocks new readers; a safe point swaps the two snapshot slots. */
    assert(catalog_guard_request_apply(&guard));
    assert(!catalog_guard_read_begin(&guard));
    assert(catalog_stage_snapshot_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active == &candidate_snapshot && candidate == &active_snapshot);
    assert(strcmp(active->records[0].path, "DVD/New.iso") == 0);
    assert(strcmp(candidate->records[0].path, "DVD/Old.iso") == 0);
    assert(!guard.apply_pending && !guard.applying);

    /* Existing readers drain before the pointer changes; the pending request is retryable. */
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_snapshot_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active == &candidate_snapshot && candidate == &active_snapshot);
    assert(catalog_guard_read_end(&guard));
    assert(catalog_stage_snapshot_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active == &active_snapshot && candidate == &candidate_snapshot);

    /* Invalid candidates cancel the pending apply and leave the active pointer untouched. */
    set_record(&candidate->records[1], 1, 0, 1, "DVD/New.iso", 300, 3);
    candidate->count = 2;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_snapshot_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_DUPLICATE);
    assert(active == &active_snapshot && candidate == &candidate_snapshot);
    assert(!guard.apply_pending && !guard.applying);

    /* Gameplay/storage activity prevents an apply request and therefore prevents publication. */
    candidate->count = 1;
    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(!catalog_guard_request_apply(&guard));
    assert(catalog_stage_snapshot_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active == &active_snapshot && candidate == &candidate_snapshot);
    assert(catalog_guard_set_activity(&guard, 0, 0));

    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}

static void test_safe_bundle_publish(void)
{
    catalog_guard guard;
    mutex_context mutex;
    catalog_stage_snapshot active_snapshot, candidate_snapshot;
    int active_items[2] = {1, 2}, candidate_items[2] = {3, 4};
    int active_sidecars[2] = {10, 20}, candidate_sidecars[2] = {30, 40};
    catalog_stage_bundle active, candidate;

    memset(&active_snapshot, 0, sizeof(active_snapshot));
    memset(&candidate_snapshot, 0, sizeof(candidate_snapshot));
    set_record(&active_snapshot.records[0], 1, 0, 1, "DVD/Old.iso", 100, 1);
    active_snapshot.count = 1;
    set_record(&candidate_snapshot.records[0], 1, 0, 1, "DVD/New.iso", 200, 2);
    candidate_snapshot.count = 1;

    active.items = active_items;
    active.sidecars = active_sidecars;
    active.count = 1;
    active.snapshot = &active_snapshot;
    candidate.items = candidate_items;
    candidate.sidecars = candidate_sidecars;
    candidate.count = 1;
    candidate.snapshot = &candidate_snapshot;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));

    /* The whole bundle stays unchanged until an explicit safe-point request. */
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(candidate.items == candidate_items && candidate.sidecars == candidate_sidecars);

    assert(catalog_guard_request_apply(&guard));
    assert(!catalog_guard_read_begin(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active.items == candidate_items && active.sidecars == candidate_sidecars);
    assert(candidate.items == active_items && candidate.sidecars == active_sidecars);
    assert(active.count == 1 && candidate.count == 1);
    assert(active.snapshot == &candidate_snapshot && candidate.snapshot == &active_snapshot);
    assert(((int *)active.items)[0] == 3 && ((int *)active.sidecars)[0] == 30);
    assert(((int *)candidate.items)[0] == 1 && ((int *)candidate.sidecars)[0] == 10);
    assert(!guard.apply_pending && !guard.applying);

    /* Existing readers prevent the entire bundle from moving; retry after drain. */
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active.items == candidate_items && active.sidecars == candidate_sidecars);
    assert(active.snapshot == &candidate_snapshot);
    assert(catalog_guard_read_end(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(active.snapshot == &active_snapshot);

    /* A duplicate candidate is rejected before any part of the active bundle changes. */
    set_record(&candidate_snapshot.records[1], 1, 0, 1, "DVD/New.iso", 300, 3);
    candidate_snapshot.count = 2;
    candidate.count = 2;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_DUPLICATE);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(active.snapshot == &active_snapshot && active.count == 1);
    assert(!guard.apply_pending && !guard.applying);

    /* An invalid active snapshot also cancels a pending apply without moving the bundle. */
    set_record(&active_snapshot.records[1], 1, 0, 1, "DVD/Old.iso", 100, 1);
    active_snapshot.count = 2;
    active.count = 2;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_DUPLICATE);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(active.snapshot == &active_snapshot && active.count == 2);
    assert(!guard.apply_pending && !guard.applying);
    active_snapshot.count = 1;
    active.count = 1;

    /* Mismatched counts and aliased buffers are rejected without touching the guard. */
    candidate_snapshot.count = 1;
    candidate.count = 2;
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_ARGUMENT);
    candidate.count = 1;
    candidate.sidecars = active.sidecars;
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(active.items == active_items && active.sidecars == active_sidecars);

    /* Gameplay/storage activity prevents a bundle apply. */
    candidate.sidecars = candidate_sidecars;
    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(!catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(catalog_guard_set_activity(&guard, 0, 0));

    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}

int main(void)
{
    test_staged_scan_builder();
    test_concurrent_scan_cancellation();
    test_safe_snapshot_publish();
    test_safe_bundle_publish();
    test_path_and_validation();
    test_snapshot_append();
    test_snapshot_index_map();
    test_sidecar_migration();
    test_diff_detects_large_file_size_changes();
    test_diff_and_stable_selection();
    test_guard_and_concurrent_reader();
    test_download_gate();
    puts("catalog stage: staged scan, append, map, sidecar migration, path, validation, diff, selection, safe snapshot publish, concurrent apply and download gates PASS");
    return 0;
}
