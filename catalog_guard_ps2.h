#ifndef TEAG3_CATALOG_GUARD_PS2_H
#define TEAG3_CATALOG_GUARD_PS2_H

#include "catalog_stage.h"

typedef struct {
    int sema_id;
    volatile int faulted;
} catalog_guard_ps2_context;

/* Call once before init; this makes an uninitialized context explicit. */
void catalog_guard_ps2_context_init(catalog_guard_ps2_context *context);

/* Create a PS2 EE binary semaphore and bind it to the staged-catalog guard. */
int catalog_guard_ps2_init(catalog_guard *guard,
                           catalog_guard_ps2_context *context);

/*
 * Destroy only after all callers have stopped using the guard. The function
 * also refuses while readers, scans, downloads, applies, gameplay or storage
 * I/O remain active. The launcher uses it for render-frame and worker leases
 * plus activity reporting; the candidate/apply path is present but the periodic
 * monitor remains disabled until target concurrency and device-removal tests pass.
 */
int catalog_guard_ps2_destroy(catalog_guard *guard,
                              catalog_guard_ps2_context *context);

/* Callback for load_games_into(): a failed semaphore read is treated as cancellation. */
int catalog_guard_ps2_scan_cancelled(void *guard_context);

#endif
