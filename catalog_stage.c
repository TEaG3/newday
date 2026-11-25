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

int catalog_stage_snapshot_diff(const catalog_stage_snapshot *old_snapshot,
                                const catalog_stage_snapshot *new_snapshot,
                                catalog_stage_diff *out)
{
    uint16_t slots[CATALOG_STAGE_HASH_SLOTS];
    unsigned char matched[CATALOG_STAGE_MAX_ITEMS];
    catalog_stage_diff result;
    size_t i;
    int status;

    if (!old_snapshot || !new_snapshot || !out) return CATALOG_STAGE_ERR_ARGUMENT;
    status = catalog_stage_snapshot_validate(old_snapshot);
    if (status != CATALOG_STAGE_OK) return status;
    status = build_index(new_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;
    status = build_index(old_snapshot, slots);
    if (status != CATALOG_STAGE_OK) return status;

    memset(&result, 0, sizeof(result));
    memset(matched, 0, sizeof(matched));
    for (i = 0; i < new_snapshot->count; ++i) {
        const catalog_stage_record *next = &new_snapshot->records[i];
        int old_index = index_find(old_snapshot, slots, next);
        if (old_index < 0) {
            result.added[result.added_count++] = (uint16_t)i;
        } else {
            const catalog_stage_record *old = &old_snapshot->records[old_index];
            matched[old_index] = 1;
            if (old->flags != next->flags || old->size != next->size ||
                old->fingerprint != next->fingerprint) {
                result.changed_old[result.changed_count] = (uint16_t)old_index;
                result.changed_new[result.changed_count] = (uint16_t)i;
                ++result.changed_count;
            }
        }
    }
    for (i = 0; i < old_snapshot->count; ++i)
        if (!matched[i]) result.removed[result.removed_count++] = (uint16_t)i;

    *out = result;
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

int catalog_guard_init(catalog_guard *guard, catalog_guard_lock_fn lock,
                       catalog_guard_lock_fn unlock, void *lock_context)
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
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    allowed = !guard->apply_pending && !guard->applying && guard->readers < UINT_MAX;
    if (allowed) ++guard->readers;
    guard->unlock(guard->lock_context);
    return allowed;
}

int catalog_guard_read_end(catalog_guard *guard)
{
    int released = 0;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    if (guard->readers) --guard->readers, released = 1;
    guard->unlock(guard->lock_context);
    return released;
}

int catalog_guard_can_scan(catalog_guard *guard)
{
    int allowed;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    allowed = !guard->playing && !guard->io_busy && !guard->scanning &&
              !guard->apply_pending && !guard->applying && guard->readers == 0;
    guard->unlock(guard->lock_context);
    return allowed;
}

int catalog_guard_scan_begin(catalog_guard *guard)
{
    int allowed;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    allowed = !guard->playing && !guard->io_busy && !guard->scanning &&
              !guard->apply_pending && !guard->applying && guard->readers == 0;
    if (allowed) guard->scanning = 1, guard->scan_cancelled = 0;
    guard->unlock(guard->lock_context);
    return allowed;
}

int catalog_guard_scan_cancelled(catalog_guard *guard)
{
    int cancelled;
    if (!guard_valid(guard)) return -1;
    guard->lock(guard->lock_context);
    cancelled = guard->scan_cancelled != 0;
    guard->unlock(guard->lock_context);
    return cancelled;
}

int catalog_guard_scan_end(catalog_guard *guard)
{
    int ended = 0;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    if (guard->scanning) {
        ended = !guard->scan_cancelled;
        guard->scanning = 0;
        guard->scan_cancelled = 0;
    }
    guard->unlock(guard->lock_context);
    return ended;
}

int catalog_guard_set_activity(catalog_guard *guard, int playing, int io_busy)
{
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    if (guard->applying) {
        guard->unlock(guard->lock_context);
        return 0;
    }
    guard->playing = playing != 0;
    guard->io_busy = io_busy != 0;
    if (guard->scanning && (guard->playing || guard->io_busy))
        guard->scan_cancelled = 1;
    guard->unlock(guard->lock_context);
    return 1;
}

int catalog_guard_request_apply(catalog_guard *guard)
{
    int requested = 0;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    if (!guard->applying && !guard->scanning) {
        guard->apply_pending = 1;
        requested = 1;
    }
    guard->unlock(guard->lock_context);
    return requested;
}

int catalog_guard_try_apply(catalog_guard *guard)
{
    int acquired = 0;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    if (guard->apply_pending && !guard->applying && !guard->scanning &&
        !guard->playing && !guard->io_busy && guard->readers == 0) {
        guard->applying = 1;
        acquired = 1;
    }
    guard->unlock(guard->lock_context);
    return acquired;
}

int catalog_guard_finish_apply(catalog_guard *guard)
{
    int finished = 0;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    if (guard->applying) {
        guard->applying = 0;
        guard->apply_pending = 0;
        finished = 1;
    }
    guard->unlock(guard->lock_context);
    return finished;
}

int catalog_guard_cancel_apply(catalog_guard *guard)
{
    int cancelled = 0;
    if (!guard_valid(guard)) return 0;
    guard->lock(guard->lock_context);
    if (!guard->applying) {
        guard->apply_pending = 0;
        cancelled = 1;
    }
    guard->unlock(guard->lock_context);
    return cancelled;
}
