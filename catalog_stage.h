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
    uint64_t size; /* bytes, or KiB when the HDL-size flag is set */
    uint32_t fingerprint;
    char path[CATALOG_STAGE_PATH_MAX];
} catalog_stage_record;

typedef struct {
    size_t count;
    catalog_stage_record records[CATALOG_STAGE_MAX_ITEMS];
} catalog_stage_snapshot;

/* One publishable generation: item storage, per-item sidecars, count and stable snapshot
   move together while the catalog guard blocks readers at the frame safe point. */
typedef struct {
    void *items;
    size_t items_capacity; /* entry slots available in items */
    void *sidecars;
    size_t sidecars_capacity; /* entry slots available in sidecars */
    size_t count;
    catalog_stage_snapshot *snapshot;
} catalog_stage_bundle;

/* Per-game state currently kept in parallel launcher arrays. Pointer fields are
   owned by exactly one published generation; migration borrows them until swap. */
typedef struct {
    void *vmc_dir[CATALOG_STAGE_MAX_ITEMS];
    int vmc_n[CATALOG_STAGE_MAX_ITEMS];
    volatile char vmc_state[CATALOG_STAGE_MAX_ITEMS];
    void *gicon[CATALOG_STAGE_MAX_ITEMS];
    volatile char gicon_state[CATALOG_STAGE_MAX_ITEMS];
    volatile char cover_req[CATALOG_STAGE_MAX_ITEMS];
    volatile char cover_done[CATALOG_STAGE_MAX_ITEMS];
    int cover_seen[CATALOG_STAGE_MAX_ITEMS];
    volatile char ra_state[CATALOG_STAGE_MAX_ITEMS];
    short ra_count[CATALOG_STAGE_MAX_ITEMS];
    int ra_gid[CATALOG_STAGE_MAX_ITEMS];
} catalog_stage_sidecar_set;

typedef void (*catalog_stage_sidecar_release_fn)(void *resource, void *context);

/* Called under the apply gate after all readers drain and before any bundle pointer moves.
   The callback may populate candidate sidecars but must not mutate the active bundle. */
typedef int (*catalog_stage_bundle_prepare_fn)(const catalog_stage_bundle *active,
                                               catalog_stage_bundle *candidate,
                                               void *context);

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
    CATALOG_STAGE_ERR_INDEX = -5,
    CATALOG_STAGE_ERR_BUSY = -6,
    CATALOG_STAGE_ERR_GUARD = -7
};

int catalog_stage_path_is_safe(const char *path);
int catalog_stage_snapshot_validate(const catalog_stage_snapshot *snapshot);
/* Append one path-validated record; validate the completed snapshot to reject duplicate stable keys. */
int catalog_stage_snapshot_append(catalog_stage_snapshot *snapshot,
                                  const catalog_stage_record *record);
/* out must not alias either snapshot; it remains untouched when validation fails. */
/* Map each old index to the new index for the same stable key; -1 means removed. */
/* Copy per-entry state only for stable keys whose size/metadata are unchanged.
 * State buffers must not overlap (overlap is rejected); capacities are entry counts, not byte counts.
 * Changed/added entries are zeroed; removed entries have no destination.
 */
int catalog_stage_sidecar_migrate(const catalog_stage_snapshot *old_snapshot,
                                  const catalog_stage_snapshot *new_snapshot,
                                  const void *old_state, size_t old_state_capacity,
                                  void *new_state, size_t new_state_capacity,
                                  size_t state_size);

/* Typed launcher sidecars: only unchanged stable keys are copied. cover_req is
   deliberately reset because it is a transient worker queue. Candidate pointer
   fields must be empty; old resources remain owned by old_state until retirement. */
int catalog_stage_sidecar_set_migrate(const catalog_stage_snapshot *old_snapshot,
                                      const catalog_stage_snapshot *new_snapshot,
                                      const catalog_stage_sidecar_set *old_state,
                                      size_t old_state_capacity,
                                      catalog_stage_sidecar_set *new_state,
                                      size_t new_state_capacity);
/* After a successful bundle swap, release old resources not transferred to new_state,
   then clear old_state. Missing release callbacks are rejected before any mutation. */
int catalog_stage_sidecar_set_retire(const catalog_stage_snapshot *old_snapshot,
                                     const catalog_stage_snapshot *new_snapshot,
                                     catalog_stage_sidecar_set *old_state,
                                     size_t old_state_capacity,
                                     const catalog_stage_sidecar_set *new_state,
                                     size_t new_state_capacity,
                                     catalog_stage_sidecar_release_fn release_vmc_dir,
                                     catalog_stage_sidecar_release_fn release_gicon,
                                     void *release_context);
/* Discard an unpublished candidate whose pointer fields may alias active resources.
   This clears borrowed pointers without freeing anything. */
void catalog_stage_sidecar_set_discard(catalog_stage_sidecar_set *state);
int catalog_stage_snapshot_map(const catalog_stage_snapshot *old_snapshot,
                               const catalog_stage_snapshot *new_snapshot,
                               int16_t *old_to_new, size_t map_capacity);
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
/* Returns 1 on completion, 0 when cancelled/not active, and -1 on guard lock/release failure. */
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
/* Swap the validated inactive candidate into the active slot only after a pending
   apply request reaches a safe point with zero readers. Call from the frame thread
   between frames. On success, candidate receives the previous active snapshot. */
int catalog_stage_snapshot_publish(catalog_guard *guard,
                                   catalog_stage_snapshot **active,
                                   catalog_stage_snapshot **candidate);
/* Atomically publish one complete generation; candidate receives the previous active bundle.
   If ERR_GUARD is returned after the swap, the guard is fail-closed; do not read either bundle
   until the guard is reinitialized. */
int catalog_stage_bundle_publish_prepared(catalog_guard *guard,
                                          catalog_stage_bundle *active,
                                          catalog_stage_bundle *candidate,
                                          catalog_stage_bundle_prepare_fn prepare,
                                          void *prepare_context);
int catalog_stage_bundle_publish(catalog_guard *guard,
                                 catalog_stage_bundle *active,
                                 catalog_stage_bundle *candidate);

/* A source callback returns 1 for a record, 0 at end-of-source, and a negative value on I/O error.
   It must finish/close its current I/O before returning; cancellation is checked immediately afterward. */
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
