#include "catalog_guard_ps2.h"

#include <kernel.h>

static int catalog_guard_ps2_lock(void *opaque)
{
    catalog_guard_ps2_context *context = (catalog_guard_ps2_context *)opaque;
    return context != NULL && !context->faulted && context->sema_id >= 0 &&
           WaitSema(context->sema_id) >= 0;
}

static int catalog_guard_ps2_unlock(void *opaque)
{
    catalog_guard_ps2_context *context = (catalog_guard_ps2_context *)opaque;
    if (context == NULL || context->faulted || context->sema_id < 0) return 0;
    if (SignalSema(context->sema_id) < 0) {
        /* Do not let a caller start I/O after a failed release of the guard semaphore. */
        context->faulted = 1;
        return 0;
    }
    return 1;
}

int catalog_guard_ps2_scan_cancelled(void *guard_context)
{
    catalog_guard *guard = (catalog_guard *)guard_context;
    return guard == NULL || catalog_guard_scan_cancelled(guard) != 0;
}

void catalog_guard_ps2_context_init(catalog_guard_ps2_context *context)
{
    if (context != NULL) {
        context->sema_id = -1;
        context->faulted = 0;
    }
}

int catalog_guard_ps2_init(catalog_guard *guard,
                           catalog_guard_ps2_context *context)
{
    ee_sema_t sema;
    int sema_id;

    if (guard == NULL || context == NULL || context->sema_id != -1 || context->faulted) {
        return 0;
    }

    sema.count = 0;
    sema.max_count = 1;
    sema.init_count = 1;
    sema.wait_threads = 0;
    sema.attr = 0;
    sema.option = 0;

    sema_id = CreateSema(&sema);
    if (sema_id < 0) {
        return 0;
    }

    context->sema_id = sema_id;
    if (!catalog_guard_init(guard, catalog_guard_ps2_lock,
                            catalog_guard_ps2_unlock, context)) {
        (void)DeleteSema(sema_id);
        context->sema_id = -1;
        return 0;
    }

    return 1;
}

int catalog_guard_ps2_destroy(catalog_guard *guard,
                              catalog_guard_ps2_context *context)
{
    int idle;
    int sema_id;

    if (guard == NULL || context == NULL || context->sema_id < 0 || context->faulted ||
        guard->lock != catalog_guard_ps2_lock ||
        guard->unlock != catalog_guard_ps2_unlock ||
        guard->lock_context != context ||
        !catalog_guard_ps2_lock(context)) {
        return 0;
    }
    idle = guard->readers == 0 &&
           !guard->playing &&
           !guard->io_busy &&
           !guard->downloading &&
           !guard->scanning &&
           !guard->apply_pending &&
           !guard->applying;
    if (idle) {
        guard->lock = NULL;
        guard->unlock = NULL;
        guard->lock_context = NULL;
    }
    if (!catalog_guard_ps2_unlock(context)) {
        /* Restore the callbacks; the faulted context makes every later lock fail closed. */
        if (idle) {
            guard->lock = catalog_guard_ps2_lock;
            guard->unlock = catalog_guard_ps2_unlock;
            guard->lock_context = context;
        }
        return 0;
    }

    if (!idle) {
        return 0;
    }

    sema_id = context->sema_id;
    if (DeleteSema(sema_id) < 0) {
        /* Keep the guard usable if the kernel refuses to delete the semaphore. */
        guard->lock = catalog_guard_ps2_lock;
        guard->unlock = catalog_guard_ps2_unlock;
        guard->lock_context = context;
        return 0;
    }

    context->sema_id = -1;
    context->faulted = 0;
    return 1;
}
