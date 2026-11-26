#ifndef TEAG3_CATALOG_STAGE_H
#define TEAG3_CATALOG_STAGE_H

#include <stddef.h>
#include <stdint.h>

#define CATALOG_STAGE_MAX_ITEMS 1024u
#define CATALOG_STAGE_PATH_MAX 160u

typedef struct {
    uint8_t source_type;
    uint8_t source_unit;
    uint8_t kind;
    uint8_t flags;
    uint32_t size;
    uint32_t fingerprint;
    char path[CATALOG_STAGE_PATH_MAX];
} catalog_stage_record;

typedef struct {
    size_t count;
    catalog_stage_record records[CATALOG_STAGE_MAX_ITEMS];
} catalog_stage_snapshot;

typedef struct {
    size_t added_count;
    size_t removed_count;
    size_t changed_count;
    uint16_t added[CATALOG_STAGE_MAX_ITEMS];
    uint16_t removed[CATALOG_STAGE_MAX_ITEMS];
    uint16_t changed_old[CATALOG_STAGE_MAX_ITEMS];
    uint16_t changed_new[CATALOG_STAGE_MAX_ITEMS];
} catalog_stage_diff;

enum {
    CATALOG_STAGE_OK = 0,
    CATALOG_STAGE_ERR_ARGUMENT = -1,
    CATALOG_STAGE_ERR_CAPACITY = -2,
    CATALOG_STAGE_ERR_PATH = -3,
    CATALOG_STAGE_ERR_DUPLICATE = -4,
    CATALOG_STAGE_ERR_INDEX = -5
};

int catalog_stage_path_is_safe(const char *path);
int catalog_stage_snapshot_validate(const catalog_stage_snapshot *snapshot);
/* out must not alias either snapshot; it remains untouched when validation fails. */
int catalog_stage_snapshot_diff(const catalog_stage_snapshot *old_snapshot,
                                const catalog_stage_snapshot *new_snapshot,
                                catalog_stage_diff *out);
int catalog_stage_snapshot_find(const catalog_stage_snapshot *snapshot,
                                uint8_t source_type, uint8_t source_unit,
                                uint8_t kind, const char *path);

typedef int (*catalog_guard_lock_fn)(void *context); /* nonzero only when the lock was acquired */
typedef int (*catalog_guard_unlock_fn)(void *context); /* nonzero only when unlocked */

typedef struct {
    catalog_guard_lock_fn lock;
    catalog_guard_unlock_fn unlock;
    void *lock_context;
    unsigned readers;
    unsigned char playing;
    unsigned char io_busy;
    unsigned char downloading;
    unsigned char download_cancelled;
    unsigned char scanning;
    unsigned char scan_cancelled;
    unsigned char apply_pending;
    unsigned char applying;
} catalog_guard;

/* The caller supplies a real mutex/semaphore pair. Every guard API is serialized by it. */
int catalog_guard_init(catalog_guard *guard, catalog_guard_lock_fn lock,
                       catalog_guard_unlock_fn unlock, void *lock_context);
/* Read-only active-snapshot leases may coexist with an inactive candidate scan. */
int catalog_guard_read_begin(catalog_guard *guard);
int catalog_guard_read_end(catalog_guard *guard);
int catalog_guard_can_scan(catalog_guard *guard);
int catalog_guard_scan_begin(catalog_guard *guard);
int catalog_guard_scan_cancelled(catalog_guard *guard);
/* Returns 1 only for a completed, non-cancelled scan; 0 means discard its candidate. */
int catalog_guard_scan_end(catalog_guard *guard);
/* Returns 0 while a cancelled scan/download is still draining; retry before starting activity. */
int catalog_guard_set_activity(catalog_guard *guard, int playing, int io_busy);
/* Downloads are exclusive with scans/applies and must poll cancellation while active. */
int catalog_guard_can_download(catalog_guard *guard);
int catalog_guard_download_begin(catalog_guard *guard);
int catalog_guard_download_cancelled(catalog_guard *guard);
int catalog_guard_download_end(catalog_guard *guard);
/* Fails while gameplay or storage I/O is active. Starting either also cancels
   a pending apply so launch/readers are never blocked by a stale safe-point request. */
int catalog_guard_request_apply(catalog_guard *guard);
int catalog_guard_try_apply(catalog_guard *guard);
int catalog_guard_finish_apply(catalog_guard *guard);
int catalog_guard_cancel_apply(catalog_guard *guard);

/* A source callback returns 1 for a record, 0 at end-of-source, and a negative value on I/O error. */
typedef int (*catalog_stage_scan_next_fn)(void *context, catalog_stage_record *record);
enum {
    CATALOG_STAGE_SCAN_OK = 0,
    CATALOG_STAGE_SCAN_CANCELLED = 1,
    CATALOG_STAGE_SCAN_ERR_ARGUMENT = -1,
    CATALOG_STAGE_SCAN_ERR_BUSY = -2,
    CATALOG_STAGE_SCAN_ERR_SOURCE = -3,
    CATALOG_STAGE_SCAN_ERR_CAPACITY = -4,
    CATALOG_STAGE_SCAN_ERR_RECORD = -5,
    CATALOG_STAGE_SCAN_ERR_GUARD = -6
};
/* Builds an inactive candidate. The caller must not publish it unless this returns OK. */
int catalog_stage_scan(catalog_guard *guard, catalog_stage_scan_next_fn next,
                       void *context, catalog_stage_snapshot *candidate);

#endif
