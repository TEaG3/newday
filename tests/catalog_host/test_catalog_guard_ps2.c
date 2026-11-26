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
    if (fail_signal || sema_id != 7 || !sema_created) {
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

    puts("catalog PS2 semaphore adapter host concurrency: PASS");
    return 0;
}
