#include "catalog_stage.h"

#include <limits.h>
#include <string.h>

#define CATALOG_STAGE_HASH_SLOTS (CATALOG_STAGE_MAX_ITEMS * 2u)
#define CATALOG_STAGE_HASH_MASK (CATALOG_STAGE_HASH_SLOTS - 1u)

static int key_equal(const catalog_stage_record *a, const catalog_stage_record *b)
{
    return a->source_type == b->source_type &&
           a->source_unit == b->source_unit &&
           a->kind == b->kind &&
           strcmp(a->path, b->path) == 0;
}

static uint32_t key_hash(const catalog_stage_record *record)
{
    uint32_t hash = 2166136261u;
    const unsigned char fields[3] = {
        record->source_type, record->source_unit, record->kind
    };
    size_t i;
    const unsigned char *p;

    for (i = 0; i < sizeof(fields); ++i) {
        hash ^= fields[i];
        hash *= 16777619u;
    }
    for (p = (const unsigned char *)record->path; *p; ++p) {
        hash ^= *p;
        hash *= 16777619u;
    }
    return hash;
}

int catalog_stage_path_is_safe(const char *path)
{
    const char *component;
    const unsigned char *p;

    if (!path || !*path || *path == '/' || *path == '\\') return 0;
    component = path;
    for (p = (const unsigned char *)path;; ++p) {
        unsigned char c = *p;
        if (c == '\\' || c == ':' || (c && c < 0x20u) || c == 0x7fu) return 0;
        if (c == '/' || c == 0) {
            size_t length = (size_t)((const char *)p - component);
            if (!length) return 0;
            if ((length == 1u && component[0] == '.') ||
                (length == 2u && component[0] == '.' && component[1] == '.')) return 0;
            if (!c) return 1;
            component = (const char *)p + 1;
        }
    }
}

int catalog_stage_snapshot_append(catalog_stage_snapshot *snapshot,
                                  const catalog_stage_record *record)
{
    if (snapshot == NULL || record == NULL) return CATALOG_STAGE_ERR_ARGUMENT;
    if (snapshot->count >= CATALOG_STAGE_MAX_ITEMS) return CATALOG_STAGE_ERR_CAPACITY;
    if (memchr(record->path, 0, sizeof(record->path)) == NULL ||
        !catalog_stage_path_is_safe(record->path)) return CATALOG_STAGE_ERR_PATH;
    snapshot->records[snapshot->count++] = *record;
    return CATALOG_STAGE_OK;
}

static int build_index(const catalog_stage_snapshot *snapshot,
                       uint16_t slots[CATALOG_STAGE_HASH_SLOTS])
{
    size_t i;

    if (!snapshot) return CATALOG_STAGE_ERR_ARGUMENT;
    if (snapshot->count > CATALOG_STAGE_MAX_ITEMS) return CATALOG_STAGE_ERR_CAPACITY;
    memset(slots, 0, sizeof(uint16_t) * CATALOG_STAGE_HASH_SLOTS);

    for (i = 0; i < snapshot->count; ++i) {
        const catalog_stage_record *record = &snapshot->records[i];
        const char *end = (const char *)memchr(record->path, 0, sizeof(record->path));
        uint32_t hash;
        size_t slot, probe;

        if (!end || end == record->path || !catalog_stage_path_is_safe(record->path))
            return CATALOG_STAGE_ERR_PATH;
        hash = key_hash(record);
        slot = (size_t)hash & CATALOG_STAGE_HASH_MASK;
        for (probe = 0; probe < CATALOG_STAGE_HASH_SLOTS; ++probe) {
            uint16_t value = slots[slot];
            if (!value) {
                slots[slot] = (uint16_t)(i + 1u);
                break;
            }
            if (key_equal(&snapshot->records[value - 1u], record))
                return CATALOG_STAGE_ERR_DUPLICATE;
            slot = (slot + 1u) & CATALOG_STAGE_HASH_MASK;
        }
        if (probe == CATALOG_STAGE_HASH_SLOTS) return CATALOG_STAGE_ERR_INDEX;
    }
    return CATALOG_STAGE_OK;
}

int catalog_stage_snapshot_validate(const catalog_stage_snapshot *snapshot)
{
    uint16_t slots[CATALOG_STAGE_HASH_SLOTS];
    return build_index(snapshot, slots);
}

static int index_find(const catalog_stage_snapshot *snapshot,
                      const uint16_t slots[CATALOG_STAGE_HASH_SLOTS],
                      const catalog_stage_record *needle)
{
    uint32_t hash = key_hash(needle);
    size_t slot = (size_t)hash & CATALOG_STAGE_HASH_MASK;
    size_t probe;

    for (probe = 0; probe < CATALOG_STAGE_HASH_SLOTS; ++probe) {
        uint16_t value = slots[slot];
        if (!value) return -1;
        if (key_equal(&snapshot->records[value - 1u], needle))
            return (int)(value - 1u);
        slot = (slot + 1u) & CATALOG_STAGE_HASH_MASK;
    }
    return -1;
}

int catalog_stage_sidecar_migrate(const catalog_stage_snapshot *old_snapshot,
                                  const catalog_stage_snapshot *new_snapshot,
                                  const void *old_state, size_t old_state_capacity,
                                  void *new_state, size_t new_state_capacity,
                                  size_t state_size)
{
    uint16_t slots[CATALOG_STAGE_HASH_SLOTS];
    const unsigned char *old_bytes = (const unsigned char *)old_state;
    unsigned char *new_bytes = (unsigned char *)new_state;
    size_t i;
    size_t old_state_bytes, new_state_bytes;
    uintptr_t old_begin, new_begin;
    int status;

    if (!old_snapshot || !new_snapshot || !state_size ||
        (old_snapshot->count && !old_state) || (new_snapshot->count && !new_state))
        return CATALOG_STAGE_ERR_ARGUMENT;
    if (old_snapshot->count > old_state_capacity ||
        new_snapshot->count > new_state_capacity)
        return CATALOG_STAGE_ERR_CAPACITY;
    if (old_snapshot->count > (size_t)-1 / state_size ||
        new_snapshot->count > (size_t)-1 / state_size)
        return CATALOG_STAGE_ERR_CAPACITY;
    old_state_bytes = old_snapshot->count * state_size;
    new_state_bytes = new_snapshot->count * state_size;
    old_begin = (uintptr_t)old_state;
    new_begin = (uintptr_t)new_state;
    if (old_state_bytes && new_state_bytes) {
        if ((old_begin <= new_begin && new_begin - old_begin < old_state_bytes) ||
            (new_begin < old_begin && old_begin - new_begin < new_state_bytes))
            return CATALOG_STAGE_ERR_ARGUMENT;
    }

    /* Validate both snapshots before touching candidate state. */
    status = build_index(new_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;
    status = build_index(old_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;

    for (i = 0; i < new_snapshot->count; ++i) {
        const catalog_stage_record *next = &new_snapshot->records[i];
        unsigned char *destination = new_bytes + i * state_size;
        int old_index = index_find(old_snapshot, slots, next);

        memset(destination, 0, state_size);
        if (old_index >= 0) {
            const catalog_stage_record *previous = &old_snapshot->records[old_index];
            if (previous->flags == next->flags && previous->size == next->size &&
                previous->fingerprint == next->fingerprint) {
                memcpy(destination, old_bytes + (size_t)old_index * state_size, state_size);
            }
        }
    }
    return CATALOG_STAGE_OK;
}

static int sidecar_record_unchanged(const catalog_stage_record *old_record,
                                      const catalog_stage_record *new_record)
{
    return old_record->flags == new_record->flags &&
           old_record->size == new_record->size &&
           old_record->fingerprint == new_record->fingerprint;
}

static int sidecar_sets_overlap(const catalog_stage_sidecar_set *a,
                                  const catalog_stage_sidecar_set *b)
{
    uintptr_t a_begin = (uintptr_t)a;
    uintptr_t b_begin = (uintptr_t)b;
    if (a_begin <= b_begin) return b_begin - a_begin < sizeof(*a);
    return a_begin - b_begin < sizeof(*b);
}

static int sidecar_set_has_resources(const catalog_stage_sidecar_set *state,
                                     size_t start, size_t end)
{
    size_t i;
    for (i = start; i < end; ++i)
        if (state->vmc_dir[i] != NULL || state->gicon[i] != NULL) return 1;
    return 0;
}

void catalog_stage_sidecar_set_discard(catalog_stage_sidecar_set *state)
{
    size_t i;
    if (state == NULL) return;
    for (i = 0; i < CATALOG_STAGE_MAX_ITEMS; ++i) {
        state->vmc_dir[i] = NULL;
        state->vmc_n[i] = 0;
        state->vmc_state[i] = 0;
        state->gicon[i] = NULL;
        state->gicon_state[i] = 0;
        state->cover_req[i] = 0;
        state->cover_done[i] = 0;
        state->cover_seen[i] = 0;
        state->ra_state[i] = 0;
        state->ra_count[i] = 0;
        state->ra_gid[i] = 0;
    }
}

int catalog_stage_sidecar_set_migrate(const catalog_stage_snapshot *old_snapshot,
                                      const catalog_stage_snapshot *new_snapshot,
                                      const catalog_stage_sidecar_set *old_state,
                                      size_t old_state_capacity,
                                      catalog_stage_sidecar_set *new_state,
                                      size_t new_state_capacity)
{
    int16_t old_to_new[CATALOG_STAGE_MAX_ITEMS];
    size_t i;
    int status;

    if (old_snapshot == NULL || new_snapshot == NULL || old_state == NULL ||
        new_state == NULL || sidecar_sets_overlap(old_state, new_state) ||
        old_snapshot == new_snapshot)
        return CATALOG_STAGE_ERR_ARGUMENT;
    if (old_state_capacity > CATALOG_STAGE_MAX_ITEMS ||
        new_state_capacity > CATALOG_STAGE_MAX_ITEMS ||
        old_snapshot->count > old_state_capacity ||
        new_snapshot->count > new_state_capacity)
        return CATALOG_STAGE_ERR_CAPACITY;
    /* Never overwrite a candidate that still owns resources from an older generation. */
    if (sidecar_set_has_resources(new_state, 0, CATALOG_STAGE_MAX_ITEMS) ||
        sidecar_set_has_resources(old_state, old_snapshot->count, CATALOG_STAGE_MAX_ITEMS))
        return CATALOG_STAGE_ERR_ARGUMENT;

    status = catalog_stage_snapshot_map(old_snapshot, new_snapshot,
                                        old_to_new, CATALOG_STAGE_MAX_ITEMS);
    if (status != CATALOG_STAGE_OK) return status;

    catalog_stage_sidecar_set_discard(new_state);
    for (i = 0; i < old_snapshot->count; ++i) {
        int next_index = old_to_new[i];
        const catalog_stage_record *old_record = &old_snapshot->records[i];
        const catalog_stage_record *new_record;
        if (next_index < 0) continue;
        new_record = &new_snapshot->records[next_index];
        if (!sidecar_record_unchanged(old_record, new_record)) continue;

        new_state->vmc_dir[next_index] = old_state->vmc_dir[i];
        new_state->vmc_n[next_index] = old_state->vmc_n[i];
        new_state->vmc_state[next_index] = old_state->vmc_state[i];
        new_state->gicon[next_index] = old_state->gicon[i];
        new_state->gicon_state[next_index] = old_state->gicon_state[i];
        /* cover_req is a transient request queue and intentionally remains zero. */
        new_state->cover_done[next_index] = old_state->cover_done[i];
        new_state->cover_seen[next_index] = old_state->cover_seen[i];
        new_state->ra_state[next_index] = old_state->ra_state[i];
        new_state->ra_count[next_index] = old_state->ra_count[i];
        new_state->ra_gid[next_index] = old_state->ra_gid[i];
    }
    return CATALOG_STAGE_OK;
}

int catalog_stage_sidecar_set_retire(const catalog_stage_snapshot *old_snapshot,
                                     const catalog_stage_snapshot *new_snapshot,
                                     catalog_stage_sidecar_set *old_state,
                                     size_t old_state_capacity,
                                     const catalog_stage_sidecar_set *new_state,
                                     size_t new_state_capacity,
                                     catalog_stage_sidecar_release_fn release_vmc_dir,
                                     catalog_stage_sidecar_release_fn release_gicon,
                                     void *release_context)
{
    int16_t old_to_new[CATALOG_STAGE_MAX_ITEMS];
    size_t i;
    int status;

    if (old_snapshot == NULL || new_snapshot == NULL || old_state == NULL ||
        new_state == NULL || old_state == new_state ||
        old_snapshot == new_snapshot)
        return CATALOG_STAGE_ERR_ARGUMENT;
    if (old_state_capacity > CATALOG_STAGE_MAX_ITEMS ||
        new_state_capacity > CATALOG_STAGE_MAX_ITEMS ||
        old_snapshot->count > old_state_capacity ||
        new_snapshot->count > new_state_capacity)
        return CATALOG_STAGE_ERR_CAPACITY;
    if (sidecar_set_has_resources(old_state, old_snapshot->count, CATALOG_STAGE_MAX_ITEMS) ||
        sidecar_set_has_resources(new_state, new_snapshot->count, CATALOG_STAGE_MAX_ITEMS))
        return CATALOG_STAGE_ERR_ARGUMENT;

    status = catalog_stage_snapshot_map(old_snapshot, new_snapshot,
                                        old_to_new, CATALOG_STAGE_MAX_ITEMS);
    if (status != CATALOG_STAGE_OK) return status;

    /* Preflight all destructors before touching ownership or clearing the old set. */
    for (i = 0; i < old_snapshot->count; ++i) {
        int next_index = old_to_new[i];
        int unchanged = next_index >= 0 &&
            sidecar_record_unchanged(&old_snapshot->records[i],
                                     &new_snapshot->records[next_index]);
        int keep_vmc = unchanged && old_state->vmc_dir[i] == new_state->vmc_dir[next_index];
        int keep_icon = unchanged && old_state->gicon[i] == new_state->gicon[next_index];
        if (old_state->vmc_dir[i] != NULL && !keep_vmc && release_vmc_dir == NULL)
            return CATALOG_STAGE_ERR_ARGUMENT;
        if (old_state->gicon[i] != NULL && !keep_icon && release_gicon == NULL)
            return CATALOG_STAGE_ERR_ARGUMENT;
    }

    for (i = 0; i < old_snapshot->count; ++i) {
        int next_index = old_to_new[i];
        int unchanged = next_index >= 0 &&
            sidecar_record_unchanged(&old_snapshot->records[i],
                                     &new_snapshot->records[next_index]);
        int keep_vmc = unchanged && old_state->vmc_dir[i] == new_state->vmc_dir[next_index];
        int keep_icon = unchanged && old_state->gicon[i] == new_state->gicon[next_index];

        if (old_state->vmc_dir[i] != NULL && !keep_vmc)
            release_vmc_dir(old_state->vmc_dir[i], release_context);
        if (old_state->gicon[i] != NULL && !keep_icon)
            release_gicon(old_state->gicon[i], release_context);
    }

    catalog_stage_sidecar_set_discard(old_state);
    return CATALOG_STAGE_OK;
}

int catalog_stage_snapshot_map(const catalog_stage_snapshot *old_snapshot,
                               const catalog_stage_snapshot *new_snapshot,
                               int16_t *old_to_new, size_t map_capacity)
{
    uint16_t slots[CATALOG_STAGE_HASH_SLOTS];
    int16_t staged[CATALOG_STAGE_MAX_ITEMS];
    size_t i;
    int status;

    if (!old_snapshot || !new_snapshot || !old_to_new)
        return CATALOG_STAGE_ERR_ARGUMENT;
    if (old_snapshot->count > map_capacity)
        return CATALOG_STAGE_ERR_CAPACITY;

    status = build_index(old_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;
    status = build_index(new_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;

    for (i = 0; i < old_snapshot->count; ++i)
        staged[i] = (int16_t)index_find(new_snapshot, slots, &old_snapshot->records[i]);

    /* Commit the map only after both snapshots have passed validation. */
    memcpy(old_to_new, staged, old_snapshot->count * sizeof(staged[0]));
    return CATALOG_STAGE_OK;
}

int catalog_stage_snapshot_diff(const catalog_stage_snapshot *old_snapshot,
                                const catalog_stage_snapshot *new_snapshot,
                                catalog_stage_diff *out)
{
    uint16_t slots[CATALOG_STAGE_HASH_SLOTS];
    unsigned char matched[CATALOG_STAGE_MAX_ITEMS];
    size_t i;
    int status;

    if (!old_snapshot || !new_snapshot || !out) return CATALOG_STAGE_ERR_ARGUMENT;
    status = build_index(new_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;
    status = build_index(old_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;

    /* Validation is complete: write directly to caller storage to keep EE stack use bounded. */
    memset(out, 0, sizeof(*out));
    memset(matched, 0, sizeof(matched));
    for (i = 0; i < new_snapshot->count; ++i) {
        const catalog_stage_record *next = &new_snapshot->records[i];
        int old_index = index_find(old_snapshot, slots, next);
        if (old_index < 0) {
            out->added[out->added_count++] = (uint16_t)i;
        } else {
            const catalog_stage_record *old = &old_snapshot->records[old_index];
            matched[old_index] = 1;
            if (old->flags != next->flags || old->size != next->size ||
                old->fingerprint != next->fingerprint) {
                out->changed_old[out->changed_count] = (uint16_t)old_index;
                out->changed_new[out->changed_count] = (uint16_t)i;
                ++out->changed_count;
            }
        }
    }
    for (i = 0; i < old_snapshot->count; ++i)
        if (!matched[i]) out->removed[out->removed_count++] = (uint16_t)i;

    return CATALOG_STAGE_OK;
}

int catalog_stage_snapshot_find(const catalog_stage_snapshot *snapshot,
                                uint8_t source_type, uint8_t source_unit,
                                uint8_t kind, const char *path)
{
    size_t i;
    if (!snapshot || !path || !catalog_stage_path_is_safe(path) ||
        catalog_stage_snapshot_validate(snapshot) != CATALOG_STAGE_OK) return -1;
    for (i = 0; i < snapshot->count; ++i) {
        const catalog_stage_record *record = &snapshot->records[i];
        if (record->source_type == source_type && record->source_unit == source_unit &&
            record->kind == kind && strcmp(record->path, path) == 0) return (int)i;
    }
    return -1;
}

static int guard_valid(const catalog_guard *guard)
{
    return guard && guard->lock && guard->unlock;
}

static int guard_lock(catalog_guard *guard)
{
    return guard_valid(guard) && guard->lock(guard->lock_context);
}

static int guard_release(catalog_guard *guard)
{
    return guard_valid(guard) && guard->unlock(guard->lock_context);
}

static int guard_download_allowed(const catalog_guard *guard)
{
    return !guard->playing && !guard->io_busy && !guard->downloading &&
           !guard->scanning && !guard->apply_pending && !guard->applying &&
           guard->readers == 0;
}

int catalog_guard_init(catalog_guard *guard, catalog_guard_lock_fn lock,
                       catalog_guard_unlock_fn unlock, void *lock_context)
{
    if (!guard || !lock || !unlock) return 0;
    memset(guard, 0, sizeof(*guard));
    guard->lock = lock;
    guard->unlock = unlock;
    guard->lock_context = lock_context;
    return 1;
}

int catalog_guard_read_begin(catalog_guard *guard)
{
    int allowed;
    if (!guard_lock(guard)) return 0;
    allowed = !guard->apply_pending && !guard->applying && !guard->downloading &&
              guard->readers < UINT_MAX;
    if (allowed) ++guard->readers;
    if (!guard_release(guard)) return 0;
    return allowed;
}

int catalog_guard_read_end(catalog_guard *guard)
{
    int released = 0;
    if (!guard_lock(guard)) return 0;
    if (guard->readers) --guard->readers, released = 1;
    if (!guard_release(guard)) return 0;
    return released;
}

int catalog_guard_can_scan(catalog_guard *guard)
{
    int allowed;
    if (!guard_lock(guard)) return 0;
    allowed = !guard->playing && !guard->io_busy && !guard->downloading && !guard->scanning &&
              !guard->apply_pending && !guard->applying && guard->readers == 0;
    if (!guard_release(guard)) return 0;
    return allowed;
}

int catalog_guard_scan_begin(catalog_guard *guard)
{
    int allowed;
    if (!guard_lock(guard)) return 0;
    allowed = !guard->playing && !guard->io_busy && !guard->downloading && !guard->scanning &&
              !guard->apply_pending && !guard->applying && guard->readers == 0;
    if (allowed) guard->scanning = 1, guard->scan_cancelled = 0;
    if (!guard_release(guard)) return 0;
    return allowed;
}

int catalog_guard_scan_cancelled(catalog_guard *guard)
{
    int cancelled;
    if (!guard_lock(guard)) return -1;
    cancelled = guard->scan_cancelled != 0;
    if (!guard_release(guard)) return -1;
    return cancelled;
}

int catalog_guard_scan_end(catalog_guard *guard)
{
    int ended = 0;
    if (!guard_lock(guard)) return -1;
    if (guard->scanning) {
        ended = !guard->scan_cancelled;
        guard->scanning = 0;
        guard->scan_cancelled = 0;
    }
    if (!guard_release(guard)) return -1;
    return ended;
}

int catalog_stage_scan(catalog_guard *guard, catalog_stage_scan_next_fn next,
                       void *context, catalog_stage_snapshot *candidate)
{
    catalog_stage_record record;
    int result = CATALOG_STAGE_SCAN_OK;
    int end_result;

    if (guard == NULL || next == NULL || candidate == NULL)
        return CATALOG_STAGE_SCAN_ERR_ARGUMENT;

    candidate->count = 0;
    if (!catalog_guard_scan_begin(guard))
        return catalog_guard_scan_cancelled(guard) < 0 ?
               CATALOG_STAGE_SCAN_ERR_GUARD : CATALOG_STAGE_SCAN_ERR_BUSY;

    for (;;) {
        int cancelled = catalog_guard_scan_cancelled(guard);
        int next_result;

        if (cancelled < 0) {
            result = CATALOG_STAGE_SCAN_ERR_GUARD;
            break;
        }
        if (cancelled) {
            result = CATALOG_STAGE_SCAN_CANCELLED;
            break;
        }

        memset(&record, 0, sizeof(record));
        next_result = next(context, &record);

        cancelled = catalog_guard_scan_cancelled(guard);
        if (cancelled < 0) {
            result = CATALOG_STAGE_SCAN_ERR_GUARD;
            break;
        }
        if (cancelled) {
            result = CATALOG_STAGE_SCAN_CANCELLED;
            break;
        }
        if (next_result < 0 || next_result > 1) {
            result = CATALOG_STAGE_SCAN_ERR_SOURCE;
            break;
        }
        if (next_result == 0)
            break;
        if (candidate->count >= CATALOG_STAGE_MAX_ITEMS) {
            result = CATALOG_STAGE_SCAN_ERR_CAPACITY;
            break;
        }
        if (memchr(record.path, 0, sizeof(record.path)) == NULL ||
            !catalog_stage_path_is_safe(record.path)) {
            result = CATALOG_STAGE_SCAN_ERR_RECORD;
            break;
        }
        candidate->records[candidate->count++] = record;
    }

    if (result == CATALOG_STAGE_SCAN_OK &&
        catalog_stage_snapshot_validate(candidate) != CATALOG_STAGE_OK)
        result = CATALOG_STAGE_SCAN_ERR_RECORD;

    end_result = catalog_guard_scan_end(guard);
    if (end_result < 0)
        result = CATALOG_STAGE_SCAN_ERR_GUARD;
    else if (end_result == 0)
        result = CATALOG_STAGE_SCAN_CANCELLED;

    if (result != CATALOG_STAGE_SCAN_OK)
        candidate->count = 0;
    return result;
}

int catalog_guard_set_activity(catalog_guard *guard, int playing, int io_busy)
{
    int wait_for_scan_or_download = 0;
    if (!guard_lock(guard)) return 0;
    if (guard->applying) {
        if (!guard_release(guard)) return 0;
        return 0;
    }
    guard->playing = playing != 0;
    guard->io_busy = io_busy != 0;
    /* Never leave a pending apply blocking new readers when launch or I/O starts. */
    if (guard->apply_pending && (guard->playing || guard->io_busy))
        guard->apply_pending = 0;
    if (guard->scanning && (guard->playing || guard->io_busy)) {
        guard->scan_cancelled = 1;
        wait_for_scan_or_download = 1;
    }
    if (guard->downloading && (guard->playing || guard->io_busy)) {
        guard->download_cancelled = 1;
        wait_for_scan_or_download = 1;
    }
    if (!guard_release(guard)) return 0;
    /* Callers starting I/O or gameplay must retry after the cancelled operation ends. */
    return !wait_for_scan_or_download;
}

int catalog_guard_can_download(catalog_guard *guard)
{
    int allowed;
    if (!guard_lock(guard)) return 0;
    allowed = guard_download_allowed(guard);
    if (!guard_release(guard)) return 0;
    return allowed;
}

int catalog_guard_download_begin(catalog_guard *guard)
{
    int allowed;
    if (!guard_lock(guard)) return 0;
    allowed = guard_download_allowed(guard);
    if (allowed) guard->downloading = 1, guard->download_cancelled = 0;
    if (!guard_release(guard)) return 0;
    return allowed;
}

int catalog_guard_download_cancelled(catalog_guard *guard)
{
    int cancelled;
    if (!guard_lock(guard)) return -1;
    cancelled = guard->downloading && guard->download_cancelled;
    if (!guard_release(guard)) return -1;
    return cancelled;
}

int catalog_guard_download_end(catalog_guard *guard)
{
    int ended = 0;
    if (!guard_lock(guard)) return 0;
    if (guard->downloading) {
        guard->downloading = 0;
        guard->download_cancelled = 0;
        ended = 1;
    }
    if (!guard_release(guard)) return 0;
    return ended;
}

int catalog_guard_request_apply(catalog_guard *guard)
{
    int requested = 0;
    if (!guard_lock(guard)) return 0;
    if (!guard->applying && !guard->scanning && !guard->downloading &&
        !guard->playing && !guard->io_busy) {
        guard->apply_pending = 1;
        requested = 1;
    }
    if (!guard_release(guard)) return 0;
    return requested;
}

int catalog_guard_try_apply(catalog_guard *guard)
{
    int acquired = 0;
    if (!guard_lock(guard)) return 0;
    if (guard->apply_pending && !guard->applying && !guard->scanning &&
        !guard->playing && !guard->io_busy && !guard->downloading && guard->readers == 0) {
        guard->applying = 1;
        acquired = 1;
    }
    if (!guard_release(guard)) return 0;
    return acquired;
}

int catalog_guard_finish_apply(catalog_guard *guard)
{
    int finished = 0;
    if (!guard_lock(guard)) return 0;
    if (guard->applying) {
        guard->applying = 0;
        guard->apply_pending = 0;
        finished = 1;
    }
    if (!guard_release(guard)) return 0;
    return finished;
}

int catalog_guard_cancel_apply(catalog_guard *guard)
{
    int cancelled = 0;
    if (!guard_lock(guard)) return 0;
    if (!guard->applying) {
        guard->apply_pending = 0;
        cancelled = 1;
    }
    if (!guard_release(guard)) return 0;
    return cancelled;
}

int catalog_stage_snapshot_publish(catalog_guard *guard,
                                   catalog_stage_snapshot **active,
                                   catalog_stage_snapshot **candidate)
{
    catalog_stage_snapshot *previous;
    int status;

    if (guard == NULL || active == NULL || candidate == NULL ||
        *active == NULL || *candidate == NULL || *active == *candidate)
        return CATALOG_STAGE_ERR_ARGUMENT;

    status = catalog_stage_snapshot_validate(*candidate);
    if (status != CATALOG_STAGE_OK) {
        if (!catalog_guard_cancel_apply(guard))
            return CATALOG_STAGE_ERR_GUARD;
        return status;
    }

    /* apply_pending blocks new readers; try_apply waits for existing leases to drain. */
    /* try_apply conflates busy and lock failure; the second guarded read detects the latter. */
    if (!catalog_guard_try_apply(guard)) {
        /* try_apply is a boolean gate; probe the guard once more to distinguish
           an ordinary busy state from a failed lock/semaphore operation. */
        if (catalog_guard_scan_cancelled(guard) < 0)
            return CATALOG_STAGE_ERR_GUARD;
        return CATALOG_STAGE_ERR_BUSY;
    }

    previous = *active;
    *active = *candidate;
    *candidate = previous;

    if (!catalog_guard_finish_apply(guard))
        return CATALOG_STAGE_ERR_GUARD;
    return CATALOG_STAGE_OK;
}

int catalog_stage_bundle_publish_prepared(catalog_guard *guard,
                                          catalog_stage_bundle *active,
                                          catalog_stage_bundle *candidate,
                                          catalog_stage_bundle_prepare_fn prepare,
                                          void *prepare_context)
{
    catalog_stage_bundle previous;
    int status;

    if (guard == NULL || active == NULL || candidate == NULL || active == candidate)
        return CATALOG_STAGE_ERR_ARGUMENT;
    if (active->items == NULL || candidate->items == NULL || active->items == candidate->items ||
        active->sidecars == NULL || candidate->sidecars == NULL ||
        active->sidecars == candidate->sidecars ||
        active->snapshot == NULL || candidate->snapshot == NULL ||
        active->snapshot == candidate->snapshot ||
        active->count != active->snapshot->count ||
        candidate->count != candidate->snapshot->count) {
        if (!catalog_guard_cancel_apply(guard))
            return CATALOG_STAGE_ERR_GUARD;
        return CATALOG_STAGE_ERR_ARGUMENT;
    }
    if (active->count > CATALOG_STAGE_MAX_ITEMS ||
        candidate->count > CATALOG_STAGE_MAX_ITEMS ||
        active->count > active->items_capacity ||
        active->count > active->sidecars_capacity ||
        candidate->count > candidate->items_capacity ||
        candidate->count > candidate->sidecars_capacity) {
        if (!catalog_guard_cancel_apply(guard))
            return CATALOG_STAGE_ERR_GUARD;
        return CATALOG_STAGE_ERR_CAPACITY;
    }

    status = catalog_stage_snapshot_validate(active->snapshot);
    if (status != CATALOG_STAGE_OK) {
        if (!catalog_guard_cancel_apply(guard))
            return CATALOG_STAGE_ERR_GUARD;
        return status;
    }
    status = catalog_stage_snapshot_validate(candidate->snapshot);
    if (status != CATALOG_STAGE_OK) {
        if (!catalog_guard_cancel_apply(guard))
            return CATALOG_STAGE_ERR_GUARD;
        return status;
    }

    /* Close the reader gate before preparing index-keyed state. The callback must
       only write the inactive candidate; active resources remain owned by active. */
    if (!catalog_guard_try_apply(guard)) {
        if (catalog_guard_scan_cancelled(guard) < 0)
            return CATALOG_STAGE_ERR_GUARD;
        return CATALOG_STAGE_ERR_BUSY;
    }

    if (prepare != NULL) {
        status = prepare(active, candidate, prepare_context);
        if (status != CATALOG_STAGE_OK) {
            if (!catalog_guard_finish_apply(guard))
                return CATALOG_STAGE_ERR_GUARD;
            return status;
        }
    }

    previous = *active;
    *active = *candidate;
    *candidate = previous;

    if (!catalog_guard_finish_apply(guard))
        return CATALOG_STAGE_ERR_GUARD;
    return CATALOG_STAGE_OK;
}

int catalog_stage_bundle_publish(catalog_guard *guard,
                                 catalog_stage_bundle *active,
                                 catalog_stage_bundle *candidate)
{
    return catalog_stage_bundle_publish_prepared(guard, active, candidate, NULL, NULL);
}
