#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "catalog_guard_ps2.h"
#include <kernel.h>

static pthread_mutex_t test_sema = PTHREAD_MUTEX_INITIALIZER;
static int sema_created;
static int fail_create;
static int fail_delete;
static int fail_wait;
static int fail_signal;
static int fail_signal_after;
static int signal_calls;

int CreateSema(ee_sema_t *sema)
{
    assert(sema != NULL);
    assert(sema->count == 0);
    assert(sema->max_count == 1);
    assert(sema->init_count == 1);
    assert(sema->wait_threads == 0);
    assert(sema->attr == 0);
    assert(sema->option == 0);

    if (fail_create || sema_created) {
        return -1;
    }
    sema_created = 1;
    return 7;
}

int DeleteSema(int sema_id)
{
    int result;

    if (sema_id != 7 || !sema_created || fail_delete) {
        return -1;
    }
    result = pthread_mutex_trylock(&test_sema);
    if (result != 0) {
        return -1;
    }
    (void)pthread_mutex_unlock(&test_sema);
    sema_created = 0;
    return 0;
}

int WaitSema(int sema_id)
{
    if (fail_wait || sema_id != 7 || !sema_created) {
        return -1;
    }
    return pthread_mutex_lock(&test_sema);
}

int SignalSema(int sema_id)
{
    ++signal_calls;
    if (fail_signal || (fail_signal_after > 0 && signal_calls >= fail_signal_after) ||
        sema_id != 7 || !sema_created) {
        return -1;
    }
    return pthread_mutex_unlock(&test_sema);
}

typedef struct {
    catalog_guard *guard;
    int iterations;
} reader_args;

static void *reader_worker(void *opaque)
{
    reader_args *args = (reader_args *)opaque;
    int i;

    for (i = 0; i < args->iterations; ++i) {
        assert(catalog_guard_read_begin(args->guard));
        assert(catalog_guard_read_end(args->guard));
    }
    return NULL;
}


typedef struct {
    int calls;
    int fail_unlock_after_record;
} staged_scan_fixture;

static int staged_scan_next(void *opaque, catalog_stage_record *record)
{
    staged_scan_fixture *fixture = (staged_scan_fixture *)opaque;
    if (fixture->calls++ == 0) {
        memset(record, 0, sizeof(*record));
        record->source_type = 1;
        record->kind = 1;
        snprintf(record->path, sizeof(record->path), "DVD/scan.iso");
        return 1;
    }
    if (fixture->fail_unlock_after_record) {
        fail_signal = 1;
    }
    return 0;
}

int main(void)
{
    enum { THREADS = 4, ITERATIONS = 4000 };
    catalog_guard guard;
    catalog_guard_ps2_context context;
    pthread_t threads[THREADS];
    reader_args args;
    int i;

    catalog_guard_ps2_context_init(&context);
    fail_create = 1;
    assert(!catalog_guard_ps2_init(&guard, &context));
    assert(context.sema_id == -1);
    assert(!sema_created);

    fail_create = 0;
    assert(catalog_guard_ps2_init(&guard, &context));
    assert(context.sema_id == 7);
    assert(sema_created);
    assert(!catalog_guard_ps2_init(&guard, &context));

    /* A failed PS2 semaphore wait must fail closed without changing guard state. */
    fail_wait = 1;
    assert(!catalog_guard_read_begin(&guard));
    assert(!catalog_guard_can_scan(&guard));
    assert(catalog_guard_scan_cancelled(&guard) == -1);
    assert(guard.readers == 0);
    assert(!catalog_guard_ps2_destroy(&guard, &context));
    assert(context.sema_id == 7 && sema_created);
    fail_wait = 0;

    args.guard = &guard;
    args.iterations = ITERATIONS;
    for (i = 0; i < THREADS; ++i) {
        assert(pthread_create(&threads[i], NULL, reader_worker, &args) == 0);
    }
    for (i = 0; i < THREADS; ++i) {
        assert(pthread_join(threads[i], NULL) == 0);
    }
    assert(guard.readers == 0);

    assert(catalog_guard_read_begin(&guard));
    assert(!catalog_guard_ps2_destroy(&guard, &context));
    assert(catalog_guard_read_end(&guard));

    assert(catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_ps2_destroy(&guard, &context));
    assert(catalog_guard_scan_end(&guard));

    assert(catalog_guard_request_apply(&guard));
    assert(!catalog_guard_ps2_destroy(&guard, &context));
    assert(catalog_guard_cancel_apply(&guard));

    /* The snapshot pointer swap must work through the real PS2 semaphore adapter. */
    {
        catalog_stage_snapshot active_snapshot, candidate_snapshot;
        catalog_stage_snapshot *active = &active_snapshot;
        catalog_stage_snapshot *candidate = &candidate_snapshot;
        memset(&active_snapshot, 0, sizeof(active_snapshot));
        memset(&candidate_snapshot, 0, sizeof(candidate_snapshot));
        active_snapshot.count = 1;
        active_snapshot.records[0].source_type = 1;
        active_snapshot.records[0].kind = 1;
        snprintf(active_snapshot.records[0].path,
                 sizeof(active_snapshot.records[0].path), "DVD/Old.iso");
        candidate_snapshot.count = 1;
        candidate_snapshot.records[0].source_type = 1;
        candidate_snapshot.records[0].kind = 1;
        snprintf(candidate_snapshot.records[0].path,
                 sizeof(candidate_snapshot.records[0].path), "DVD/New.iso");
        assert(catalog_guard_request_apply(&guard));
        assert(catalog_stage_snapshot_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
        assert(active == &candidate_snapshot && candidate == &active_snapshot);
        assert(strcmp(active->records[0].path, "DVD/New.iso") == 0);
        assert(strcmp(candidate->records[0].path, "DVD/Old.iso") == 0);
    }

    /* Item, sidecar, count and snapshot slots move together through the PS2 semaphore adapter. */
    {
        catalog_stage_snapshot old_snapshot, new_snapshot;
        int old_items[1] = {1}, new_items[1] = {2};
        int old_sidecars[1] = {10}, new_sidecars[1] = {20};
        catalog_stage_bundle active_bundle, candidate_bundle;
        memset(&old_snapshot, 0, sizeof(old_snapshot));
        memset(&new_snapshot, 0, sizeof(new_snapshot));
        old_snapshot.count = 1;
        old_snapshot.records[0].source_type = 1;
        old_snapshot.records[0].kind = 1;
        snprintf(old_snapshot.records[0].path, sizeof(old_snapshot.records[0].path), "DVD/Old.iso");
        new_snapshot.count = 1;
        new_snapshot.records[0].source_type = 1;
        new_snapshot.records[0].kind = 1;
        snprintf(new_snapshot.records[0].path, sizeof(new_snapshot.records[0].path), "DVD/New.iso");
        active_bundle.items = old_items;
        active_bundle.items_capacity = 1;
        active_bundle.sidecars = old_sidecars;
        active_bundle.sidecars_capacity = 1;
        active_bundle.count = 1;
        active_bundle.snapshot = &old_snapshot;
        candidate_bundle.items = new_items;
        candidate_bundle.items_capacity = 1;
        candidate_bundle.sidecars = new_sidecars;
        candidate_bundle.sidecars_capacity = 1;
        candidate_bundle.count = 1;
        candidate_bundle.snapshot = &new_snapshot;
        assert(catalog_guard_request_apply(&guard));
        assert(catalog_stage_bundle_publish(&guard, &active_bundle, &candidate_bundle) == CATALOG_STAGE_OK);
        assert(active_bundle.items == new_items && active_bundle.sidecars == new_sidecars);
        assert(candidate_bundle.items == old_items && candidate_bundle.sidecars == old_sidecars);
        assert(active_bundle.snapshot == &new_snapshot && candidate_bundle.snapshot == &old_snapshot);
    }

    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(!catalog_guard_ps2_destroy(&guard, &context));
    assert(catalog_guard_set_activity(&guard, 0, 0));

    fail_delete = 1;
    assert(!catalog_guard_ps2_destroy(&guard, &context));
    assert(guard.lock != NULL);
    assert(context.sema_id == 7);
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));

    fail_delete = 0;
    assert(catalog_guard_ps2_destroy(&guard, &context));
    assert(context.sema_id == -1);
    assert(!sema_created);
    assert(!catalog_guard_read_begin(&guard));

    /* A failed SignalSema must be reported to the caller and poison future locks. */
    {
        catalog_guard failed_guard;
        catalog_guard_ps2_context failed_context;
        catalog_guard_ps2_context_init(&failed_context);
        assert(catalog_guard_ps2_init(&failed_guard, &failed_context));
        fail_signal = 1;
        assert(!catalog_guard_read_begin(&failed_guard));
        assert(failed_context.faulted);
        assert(failed_guard.readers == 1); /* state is no longer safely observable */
        assert(!catalog_guard_read_begin(&failed_guard)); /* no blocking WaitSema after fault */
        assert(!catalog_guard_ps2_destroy(&failed_guard, &failed_context));
        fail_signal = 0;
        /* The simulated kernel failure left the mutex held; clean up the test stub directly. */
        assert(pthread_mutex_unlock(&test_sema) == 0);
        sema_created = 0;
    }

    /* A failed release after bundle swap must fail closed; no reader may observe the bundle. */
    {
        catalog_guard failed_bundle_guard;
        catalog_guard_ps2_context failed_bundle_context;
        catalog_stage_snapshot old_snapshot, new_snapshot;
        int old_items[1] = {1}, new_items[1] = {2};
        int old_sidecars[1] = {10}, new_sidecars[1] = {20};
        catalog_stage_bundle active_bundle, candidate_bundle;
        memset(&old_snapshot, 0, sizeof(old_snapshot));
        memset(&new_snapshot, 0, sizeof(new_snapshot));
        old_snapshot.count = 1;
        old_snapshot.records[0].source_type = 1;
        old_snapshot.records[0].kind = 1;
        snprintf(old_snapshot.records[0].path, sizeof(old_snapshot.records[0].path), "DVD/Old.iso");
        new_snapshot.count = 1;
        new_snapshot.records[0].source_type = 1;
        new_snapshot.records[0].kind = 1;
        snprintf(new_snapshot.records[0].path, sizeof(new_snapshot.records[0].path), "DVD/New.iso");
        active_bundle.items = old_items;
        active_bundle.items_capacity = 1;
        active_bundle.sidecars = old_sidecars;
        active_bundle.sidecars_capacity = 1;
        active_bundle.count = 1;
        active_bundle.snapshot = &old_snapshot;
        candidate_bundle.items = new_items;
        candidate_bundle.items_capacity = 1;
        candidate_bundle.sidecars = new_sidecars;
        candidate_bundle.sidecars_capacity = 1;
        candidate_bundle.count = 1;
        candidate_bundle.snapshot = &new_snapshot;
        catalog_guard_ps2_context_init(&failed_bundle_context);
        assert(catalog_guard_ps2_init(&failed_bundle_guard, &failed_bundle_context));
        assert(catalog_guard_request_apply(&failed_bundle_guard));
        signal_calls = 0;
        fail_signal_after = 2; /* try_apply releases once; finish_apply release fails */
        assert(catalog_stage_bundle_publish(&failed_bundle_guard, &active_bundle, &candidate_bundle) == CATALOG_STAGE_ERR_GUARD);
        assert(failed_bundle_context.faulted);
        assert(active_bundle.items == new_items && active_bundle.sidecars == new_sidecars);
        assert(!catalog_guard_read_begin(&failed_bundle_guard));
        fail_signal_after = 0;
        assert(pthread_mutex_unlock(&test_sema) == 0);
        sema_created = 0;
    }

    /* Destroy must also restore its callbacks and refuse deletion when SignalSema fails. */
    {
        catalog_guard failed_destroy_guard;
        catalog_guard_ps2_context failed_destroy_context;
        catalog_guard_ps2_context_init(&failed_destroy_context);
        assert(catalog_guard_ps2_init(&failed_destroy_guard, &failed_destroy_context));
        fail_signal = 1;
        assert(!catalog_guard_ps2_destroy(&failed_destroy_guard, &failed_destroy_context));
        assert(failed_destroy_context.faulted);
        assert(failed_destroy_guard.lock != NULL);
        assert(failed_destroy_guard.unlock != NULL);
        assert(failed_destroy_guard.lock_context == &failed_destroy_context);
        assert(!catalog_guard_read_begin(&failed_destroy_guard));
        fail_signal = 0;
        assert(pthread_mutex_unlock(&test_sema) == 0);
        sema_created = 0;
    }


    /* A failed WaitSema at scan start must not call the source or expose a candidate. */
    {
        catalog_guard scan_guard;
        catalog_guard_ps2_context scan_context;
        catalog_stage_snapshot candidate;
        staged_scan_fixture fixture = {0, 0};

        catalog_guard_ps2_context_init(&scan_context);
        assert(catalog_guard_ps2_init(&scan_guard, &scan_context));
        memset(&candidate, 0, sizeof(candidate));
        fail_wait = 1;
        assert(catalog_stage_scan(&scan_guard, staged_scan_next, &fixture, &candidate) ==
               CATALOG_STAGE_SCAN_ERR_GUARD);
        assert(fixture.calls == 0);
        assert(candidate.count == 0);
        fail_wait = 0;
        assert(catalog_guard_ps2_destroy(&scan_guard, &scan_context));
    }

    /* A failed SignalSema after one staged record must discard the partial candidate. */
    {
        catalog_guard scan_guard;
        catalog_guard_ps2_context scan_context;
        catalog_stage_snapshot candidate;
        staged_scan_fixture fixture = {0, 1};

        catalog_guard_ps2_context_init(&scan_context);
        assert(catalog_guard_ps2_init(&scan_guard, &scan_context));
        memset(&candidate, 0, sizeof(candidate));
        assert(catalog_stage_scan(&scan_guard, staged_scan_next, &fixture, &candidate) ==
               CATALOG_STAGE_SCAN_ERR_GUARD);
        assert(fixture.calls == 2);
        assert(candidate.count == 0);
        assert(scan_context.faulted);
        assert(!catalog_guard_ps2_destroy(&scan_guard, &scan_context));
        fail_signal = 0;
        /* The simulated SignalSema failure leaves the host mutex locked. */
        assert(pthread_mutex_unlock(&test_sema) == 0);
        sema_created = 0;
    }

    /* The exported callback treats guard lock failures and requested scan cancellation as stop signals. */
    {
        catalog_guard callback_guard;
        catalog_guard_ps2_context callback_context;
        catalog_guard_ps2_context_init(&callback_context);
        assert(catalog_guard_ps2_init(&callback_guard, &callback_context));
        assert(catalog_guard_scan_begin(&callback_guard));
        assert(!catalog_guard_ps2_scan_cancelled(&callback_guard));
        fail_wait = 1;
        assert(catalog_guard_ps2_scan_cancelled(&callback_guard));
        fail_wait = 0;
        assert(!catalog_guard_set_activity(&callback_guard, 1, 0));
        assert(catalog_guard_ps2_scan_cancelled(&callback_guard));
        assert(!catalog_guard_scan_end(&callback_guard));
        assert(catalog_guard_set_activity(&callback_guard, 0, 0));
        assert(catalog_guard_ps2_destroy(&callback_guard, &callback_context));
    }

    puts("catalog PS2 semaphore adapter host concurrency: PASS");
    return 0;
}
