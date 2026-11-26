#include "catalog_stage.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

static catalog_stage_snapshot old_snapshot;
static catalog_stage_snapshot new_snapshot;

static void set_record(catalog_stage_record *record, uint8_t source,
                       uint8_t unit, uint8_t kind, const char *path,
                       uint32_t size, uint32_t fingerprint)
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

int main(void)
{
    test_staged_scan_builder();
    test_path_and_validation();
    test_diff_and_stable_selection();
    test_guard_and_concurrent_reader();
    test_download_gate();
    puts("catalog stage: staged scan, path, validation, diff, selection, concurrent apply and download gates PASS");
    return 0;
}
