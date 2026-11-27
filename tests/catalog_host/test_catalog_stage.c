#include "catalog_stage.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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

typedef struct {
    int vmc_releases;
    int icon_releases;
    int cover_releases;
} sidecar_release_counts;

static catalog_stage_sidecar_set sidecar_old;
static catalog_stage_sidecar_set sidecar_new;

static void init_sidecar_set(catalog_stage_sidecar_set *state)
{
    memset(state, 0, sizeof(*state));
    state->icon_want = -1;
    state->vmc_net_want = -1;
    state->ra_want = -1;
    state->ra_list_want = -1;
    state->ra_list_for = -1;
}

static void release_vmc_resource(void *resource, void *opaque)
{
    sidecar_release_counts *counts = (sidecar_release_counts *)opaque;
    assert(resource != NULL && counts != NULL);
    ++counts->vmc_releases;
    free(resource);
}

static void release_icon_resource(void *resource, void *opaque)
{
    sidecar_release_counts *counts = (sidecar_release_counts *)opaque;
    assert(resource != NULL && counts != NULL);
    ++counts->icon_releases;
    free(resource);
}

static void release_cover_resource(void *resource, void *opaque)
{
    sidecar_release_counts *counts = (sidecar_release_counts *)opaque;
    assert(resource != NULL && counts != NULL);
    ++counts->cover_releases;
    free(resource);
}

static void *test_resource(void)
{
    void *resource = malloc(1);
    assert(resource != NULL);
    return resource;
}

static void test_typed_sidecar_set_migration_and_retirement(void)
{
    catalog_stage_snapshot old_snapshot, new_snapshot;
    sidecar_release_counts releases = {0, 0, 0};
    void *old_a_vmc, *old_a_icon;
    void *old_a_cover_big, *old_a_cover_small, *old_a_cover_half;
    void *old_busy_cover_half;

    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    init_sidecar_set(&sidecar_old);
    init_sidecar_set(&sidecar_new);

    old_snapshot.count = 4;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&old_snapshot.records[1], 1, 0, 0, "DVD/Beta.iso", 200, 22);
    set_record(&old_snapshot.records[2], 2, 0, 0, "HDL/Removed", 300, 33);
    set_record(&old_snapshot.records[3], 1, 0, 0, "DVD/Busy.iso", 400, 44);

    new_snapshot.count = 4;
    set_record(&new_snapshot.records[0], 1, 0, 0, "DVD/Beta.iso", 201, 23); /* same key, changed content */
    set_record(&new_snapshot.records[1], 1, 0, 0, "DVD/Alpha.iso", 100, 11); /* unchanged, reordered */
    set_record(&new_snapshot.records[2], 1, 0, 0, "DVD/Added.iso", 400, 44);
    set_record(&new_snapshot.records[3], 1, 0, 0, "DVD/Busy.iso", 400, 44);

    old_a_vmc = sidecar_old.vmc_dir[0] = test_resource();
    old_a_icon = sidecar_old.gicon[0] = test_resource();
    old_a_cover_big = sidecar_old.cover_big[0] = test_resource();
    old_a_cover_small = sidecar_old.cover_small[0] = test_resource();
    old_a_cover_half = sidecar_old.cover_half[0] = test_resource();
    sidecar_old.vmc_n[0] = 8;
    sidecar_old.vmc_state[0] = 2;
    sidecar_old.gicon_state[0] = 2;
    sidecar_old.cover_req[0] = 1;
    sidecar_old.cover_done[0] = 3;
    sidecar_old.cover_seen[0] = 22;
    sidecar_old.ra_state[0] = 4;
    sidecar_old.ra_count[0] = 7;
    sidecar_old.ra_gid[0] = 1234;

    sidecar_old.vmc_dir[1] = test_resource();
    sidecar_old.gicon[1] = test_resource();
    sidecar_old.cover_big[1] = test_resource();
    sidecar_old.vmc_n[1] = 16;
    sidecar_old.ra_count[1] = 9;
    sidecar_old.vmc_dir[2] = test_resource();
    sidecar_old.gicon[2] = test_resource();
    sidecar_old.cover_small[2] = test_resource();
    sidecar_old.vmc_dir[3] = test_resource();
    sidecar_old.gicon[3] = test_resource();
    old_busy_cover_half = sidecar_old.cover_half[3] = test_resource();
    sidecar_old.vmc_state[3] = 1;
    sidecar_old.gicon_state[3] = 1;
    sidecar_old.ra_state[3] = 1;
    sidecar_old.vmc_n[3] = 64;
    sidecar_old.ra_count[3] = 5;
    sidecar_old.ra_gid[3] = 55;
    sidecar_old.icon_want = 0;       /* Alpha is unchanged and moves to index 1. */
    sidecar_old.vmc_net_want = 1;    /* Beta changed content; cancel the stale request. */
    sidecar_old.ra_want = 2;         /* Removed game; cancel the stale request. */
    sidecar_old.ra_list_want = 3;    /* Busy is unchanged and remains at index 3. */
    sidecar_old.ra_list_for = 0;     /* Cached list still belongs to Alpha at index 1. */

    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 4, &sidecar_new, 4) == CATALOG_STAGE_OK);
    assert(sidecar_new.vmc_dir[1] == old_a_vmc && sidecar_new.gicon[1] == old_a_icon);
    assert(sidecar_new.cover_big[1] == old_a_cover_big);
    assert(sidecar_new.cover_small[1] == old_a_cover_small);
    assert(sidecar_new.cover_half[1] == old_a_cover_half);
    assert(sidecar_new.cover_half[3] == old_busy_cover_half);
    assert(sidecar_new.vmc_n[1] == 8 && sidecar_new.vmc_state[1] == 2);
    assert(sidecar_new.gicon_state[1] == 2);
    assert(sidecar_new.cover_req[1] == 0); /* pending worker request is never migrated */
    assert(sidecar_new.cover_done[1] == 3 && sidecar_new.cover_seen[1] == 22);
    assert(sidecar_new.ra_state[1] == 4 && sidecar_new.ra_count[1] == 7 && sidecar_new.ra_gid[1] == 1234);
    assert(sidecar_new.icon_want == 1);
    assert(sidecar_new.vmc_net_want == -1 && sidecar_new.ra_want == -1);
    assert(sidecar_new.ra_list_want == 3 && sidecar_new.ra_list_for == 1);
    assert(sidecar_new.vmc_dir[0] == NULL && sidecar_new.gicon[0] == NULL);
    assert(sidecar_new.vmc_n[0] == 0 && sidecar_new.ra_count[0] == 0);
    assert(sidecar_new.vmc_dir[2] == NULL && sidecar_new.gicon[2] == NULL);
    assert(sidecar_new.vmc_dir[3] == NULL && sidecar_new.gicon[3] == NULL);
    assert(sidecar_new.vmc_state[3] == 0 && sidecar_new.gicon_state[3] == 0 && sidecar_new.ra_state[3] == 0);
    assert(sidecar_new.vmc_n[3] == 0 && sidecar_new.ra_count[3] == 0 && sidecar_new.ra_gid[3] == 0);

    assert(catalog_stage_sidecar_set_retire(&old_snapshot, &new_snapshot,
                                            &sidecar_old, 4, &sidecar_new, 4,
                                            release_vmc_resource, release_icon_resource, release_cover_resource,
                                            &releases) == CATALOG_STAGE_OK);
    assert(releases.vmc_releases == 3 && releases.icon_releases == 3 && releases.cover_releases == 2);
    assert(sidecar_old.vmc_dir[0] == NULL && sidecar_old.gicon[0] == NULL);
    assert(sidecar_old.vmc_dir[1] == NULL && sidecar_old.gicon[1] == NULL);
    assert(sidecar_old.vmc_dir[2] == NULL && sidecar_old.gicon[2] == NULL);
    assert(sidecar_old.cover_big[0] == NULL && sidecar_old.cover_small[0] == NULL && sidecar_old.cover_half[0] == NULL);
    assert(sidecar_old.cover_big[1] == NULL && sidecar_old.cover_small[2] == NULL && sidecar_old.cover_half[3] == NULL);
    assert(sidecar_new.vmc_dir[1] == old_a_vmc && sidecar_new.gicon[1] == old_a_icon);

    free(sidecar_new.vmc_dir[1]);
    free(sidecar_new.gicon[1]);
    free(sidecar_new.cover_big[1]);
    free(sidecar_new.cover_small[1]);
    free(sidecar_new.cover_half[1]);
    free(sidecar_new.cover_half[3]);
    catalog_stage_sidecar_set_discard(&sidecar_new);
}

static void test_typed_sidecar_set_discard_and_fail_closed(void)
{
    catalog_stage_snapshot old_snapshot, new_snapshot;
    sidecar_release_counts releases = {0, 0, 0};
    void *vmc_resource, *icon_resource, *cover_resource;

    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    init_sidecar_set(&sidecar_old);
    init_sidecar_set(&sidecar_new);
    old_snapshot.count = 1;
    new_snapshot.count = 1;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Keep.iso", 100, 7);
    set_record(&new_snapshot.records[0], 1, 0, 0, "DVD/Keep.iso", 100, 7);
    sidecar_old.icon_want = 1; /* Out-of-range queued indices reject migration before touching the candidate. */
    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 1, &sidecar_new, 1) == CATALOG_STAGE_ERR_INDEX);
    assert(sidecar_new.icon_want == -1 && sidecar_new.vmc_dir[0] == NULL);
    sidecar_old.icon_want = -1;
    vmc_resource = sidecar_old.vmc_dir[0] = test_resource();
    icon_resource = sidecar_old.gicon[0] = test_resource();
    cover_resource = sidecar_old.cover_big[0] = test_resource();

    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 1, &sidecar_new, 1) == CATALOG_STAGE_OK);
    catalog_stage_sidecar_set_discard(&sidecar_new); /* candidate aliases active resources */
    assert(sidecar_new.vmc_dir[0] == NULL && sidecar_new.gicon[0] == NULL && sidecar_new.cover_big[0] == NULL);
    assert(sidecar_new.icon_want == -1 && sidecar_new.vmc_net_want == -1 && sidecar_new.ra_want == -1);
    assert(sidecar_new.ra_list_want == -1 && sidecar_new.ra_list_for == -1);
    assert(sidecar_old.vmc_dir[0] == vmc_resource && sidecar_old.gicon[0] == icon_resource && sidecar_old.cover_big[0] == cover_resource);
    assert(releases.vmc_releases == 0 && releases.icon_releases == 0 && releases.cover_releases == 0);
    free(vmc_resource);
    free(icon_resource);
    free(cover_resource);

    init_sidecar_set(&sidecar_old);
    init_sidecar_set(&sidecar_new);
    sidecar_new.cover_big[4] = (void *)1;
    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 1, &sidecar_new, 1) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(sidecar_new.cover_big[4] == (void *)1);
    catalog_stage_sidecar_set_discard(&sidecar_new);

    /* Stale pointers outside the old snapshot must be retired before migration. */
    sidecar_old.cover_small[5] = test_resource();
    icon_resource = sidecar_old.cover_small[5];
    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 1, &sidecar_new, 1) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(sidecar_old.cover_small[5] == icon_resource);
    free(icon_resource);
    catalog_stage_sidecar_set_discard(&sidecar_old);

    /* Partial overlap is rejected before either sidecar generation is touched. */
    {
        union {
            void *align;
            unsigned char bytes[sizeof(catalog_stage_sidecar_set) * 2];
        } storage;
        catalog_stage_sidecar_set *overlap_old = (catalog_stage_sidecar_set *)(void *)storage.bytes;
        catalog_stage_sidecar_set *overlap_new =
            (catalog_stage_sidecar_set *)(void *)(storage.bytes + sizeof(catalog_stage_sidecar_set) - sizeof(void *));
        memset(&storage, 0, sizeof(storage));
        assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                                 overlap_old, 1, overlap_new, 1) == CATALOG_STAGE_ERR_ARGUMENT);
    }

    new_snapshot.records[0].path[0] = 'X';
    new_snapshot.records[1] = new_snapshot.records[0];
    new_snapshot.count = 2;
    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 1, &sidecar_new, 2) == CATALOG_STAGE_ERR_DUPLICATE);
    assert(sidecar_new.gicon[0] == NULL && sidecar_new.vmc_dir[0] == NULL);

    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    init_sidecar_set(&sidecar_old);
    old_snapshot.count = 1;
    new_snapshot.count = 0;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Removed.iso", 100, 8);
    sidecar_old.cover_half[0] = test_resource();
    icon_resource = sidecar_old.cover_half[0];
    assert(catalog_stage_sidecar_set_retire(&old_snapshot, &new_snapshot,
                                            &sidecar_old, 1, &sidecar_new, 0,
                                            NULL, NULL, NULL, &releases) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(sidecar_old.cover_half[0] == icon_resource); /* missing destructor never loses ownership */
    assert(catalog_stage_sidecar_set_retire(&old_snapshot, &new_snapshot,
                                            &sidecar_old, 1, &sidecar_new, 0,
                                            release_vmc_resource, release_icon_resource, release_cover_resource,
                                            &releases) == CATALOG_STAGE_OK);
    assert(releases.cover_releases == 1);
}

static void test_typed_sidecar_set_rejects_aliases_and_unexpected_borrows(void)
{
    catalog_stage_snapshot old_snapshot, new_snapshot;
    sidecar_release_counts releases = {0, 0, 0};
    void *shared, *vmc, *icon, *rogue;

    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    init_sidecar_set(&sidecar_old);
    init_sidecar_set(&sidecar_new);
    old_snapshot.count = 2;
    new_snapshot.count = 2;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&old_snapshot.records[1], 1, 0, 0, "DVD/Beta.iso", 200, 22);
    set_record(&new_snapshot.records[0], 1, 0, 0, "DVD/Alpha.iso", 100, 11);
    set_record(&new_snapshot.records[1], 1, 0, 0, "DVD/Beta.iso", 200, 22);

    /* One allocation cannot be owned by two entries or resource classes. */
    shared = test_resource();
    sidecar_old.vmc_dir[0] = shared;
    sidecar_old.gicon[1] = shared;
    sidecar_old.cover_big[1] = shared;
    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 2, &sidecar_new, 2) ==
           CATALOG_STAGE_ERR_ARGUMENT);
    assert(sidecar_old.vmc_dir[0] == shared && sidecar_old.gicon[1] == shared && sidecar_old.cover_big[1] == shared);
    assert(sidecar_new.vmc_dir[0] == NULL && sidecar_new.gicon[1] == NULL);
    free(shared);

    init_sidecar_set(&sidecar_old);
    init_sidecar_set(&sidecar_new);
    old_snapshot.count = 1;
    new_snapshot.count = 1;
    set_record(&old_snapshot.records[0], 1, 0, 0, "DVD/Keep.iso", 100, 7);
    set_record(&new_snapshot.records[0], 1, 0, 0, "DVD/Keep.iso", 100, 7);
    vmc = sidecar_old.vmc_dir[0] = test_resource();
    icon = sidecar_old.gicon[0] = test_resource();
    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 1, &sidecar_new, 1) ==
           CATALOG_STAGE_OK);

    /* A foreign candidate pointer must never survive retirement of the old generation. */
    rogue = test_resource();
    sidecar_new.cover_big[0] = rogue;
    assert(catalog_stage_sidecar_set_retire(&old_snapshot, &new_snapshot,
                                            &sidecar_old, 1, &sidecar_new, 1,
                                            release_vmc_resource, release_icon_resource, release_cover_resource,
                                            &releases) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(sidecar_old.vmc_dir[0] == vmc && sidecar_old.gicon[0] == icon);
    assert(releases.vmc_releases == 0 && releases.icon_releases == 0 && releases.cover_releases == 0);
    sidecar_new.cover_big[0] = NULL;
    free(rogue);

    /* Aliased candidate slots are rejected before any resource can be released. */
    sidecar_new.cover_big[0] = sidecar_new.vmc_dir[0];
    assert(catalog_stage_sidecar_set_retire(&old_snapshot, &new_snapshot,
                                            &sidecar_old, 1, &sidecar_new, 1,
                                            release_vmc_resource, release_icon_resource, release_cover_resource,
                                            &releases) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(sidecar_old.vmc_dir[0] == vmc && sidecar_old.gicon[0] == icon);
    assert(releases.vmc_releases == 0 && releases.icon_releases == 0);

    sidecar_new.cover_big[0] = NULL;
    sidecar_new.gicon[0] = icon;
    assert(catalog_stage_sidecar_set_retire(&old_snapshot, &new_snapshot,
                                            &sidecar_old, 1, &sidecar_new, 1,
                                            release_vmc_resource, release_icon_resource, release_cover_resource,
                                            &releases) == CATALOG_STAGE_OK);
    assert(sidecar_old.vmc_dir[0] == NULL && sidecar_old.gicon[0] == NULL);
    assert(sidecar_new.vmc_dir[0] == vmc && sidecar_new.gicon[0] == icon);
    assert(releases.vmc_releases == 0 && releases.icon_releases == 0);
    free(vmc);
    free(icon);
    catalog_stage_sidecar_set_discard(&sidecar_new);
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
    assert(catalog_guard_can_scan(&guard)); /* active readers do not block an inactive candidate scan */
    assert(catalog_guard_scan_begin(&guard));
    assert(!catalog_guard_can_scan(&guard));
    assert(catalog_guard_read_end(&guard)); /* the original lease can drain while scanning */
    assert(catalog_guard_read_begin(&guard)); /* new render readers may also read the active snapshot */
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

    /* Read-only active-snapshot leases may coexist with an inactive candidate scan. */
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_can_scan(&guard));
    fixture.index = 0;
    candidate.count = 9;
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate) == CATALOG_STAGE_SCAN_OK);
    assert(candidate.count == 2);
    assert(guard.readers == 1); /* the active reader remains protected throughout the scan */

    /* Apply still waits for the reader to drain and blocks new readers once requested. */
    assert(catalog_guard_request_apply(&guard));
    assert(!catalog_guard_can_scan(&guard));
    assert(!catalog_guard_try_apply(&guard));
    assert(!catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(catalog_guard_try_apply(&guard));
    assert(catalog_guard_finish_apply(&guard));

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

typedef struct {
    catalog_guard *guard;
    int fail;
    int calls;
} bundle_prepare_context;

static int migrate_bundle_sidecars(const catalog_stage_bundle *active,
                                   catalog_stage_bundle *candidate,
                                   void *opaque)
{
    bundle_prepare_context *context = (bundle_prepare_context *)opaque;
    int status;

    assert(context != NULL && context->guard != NULL);
    assert(context->guard->applying);
    assert(context->guard->readers == 0);
    ++context->calls;

    status = catalog_stage_sidecar_migrate(active->snapshot, candidate->snapshot,
                                           active->sidecars, active->sidecars_capacity,
                                           candidate->sidecars, candidate->sidecars_capacity,
                                           sizeof(int));
    if (status != CATALOG_STAGE_OK) return status;
    return context->fail ? CATALOG_STAGE_ERR_ARGUMENT : CATALOG_STAGE_OK;
}

static void test_bundle_selection_rebase(void)
{
    catalog_guard guard;
    mutex_context mutex;
    catalog_stage_snapshot active_snapshot, candidate_snapshot;
    int old_items[2] = {1, 2}, new_items[2] = {2, 1};
    int old_sidecars[2] = {10, 20}, new_sidecars[2] = {20, 10};
    catalog_stage_bundle active, candidate;

    memset(&active_snapshot, 0, sizeof(active_snapshot));
    memset(&candidate_snapshot, 0, sizeof(candidate_snapshot));
    set_record(&active_snapshot.records[0], 1, 0, 1, "DVD/Alpha.iso", 100, 11);
    set_record(&active_snapshot.records[1], 1, 0, 1, "DVD/Beta.iso", 200, 22);
    active_snapshot.count = 2;
    set_record(&candidate_snapshot.records[0], 1, 0, 1, "DVD/Beta.iso", 200, 22);
    set_record(&candidate_snapshot.records[1], 1, 0, 1, "DVD/Alpha.iso", 150, 99); /* same key, changed content */
    candidate_snapshot.count = 2;

    active.items = old_items;
    active.items_capacity = 2;
    active.sidecars = old_sidecars;
    active.sidecars_capacity = 2;
    active.count = 2;
    active.selected_index = 0;
    active.snapshot = &active_snapshot;
    candidate.items = new_items;
    candidate.items_capacity = 2;
    candidate.sidecars = new_sidecars;
    candidate.sidecars_capacity = 2;
    candidate.count = 2;
    candidate.selected_index = -1;
    candidate.snapshot = &candidate_snapshot;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active.selected_index == 1); /* stable key survives reorder and content change */

    /* If the selected key disappears, publish a valid first-item fallback. */
    set_record(&active_snapshot.records[0], 1, 0, 1, "DVD/Beta.iso", 200, 22);
    active_snapshot.count = 1;
    candidate.count = 1;
    active.selected_index = 1; /* Alpha is selected in the active reordered snapshot. */
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active.selected_index == 0);

    /* An empty candidate clears the selection rather than retaining an invalid index. */
    candidate_snapshot.count = 0;
    candidate.count = 0;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active.selected_index == -1);

    /* An invalid active index is rejected before any pointer or count moves. */
    active.selected_index = 4;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_INDEX);
    assert(active.items == new_items && active.sidecars == new_sidecars);
    assert(active.selected_index == 4);
    assert(!guard.apply_pending && !guard.applying);
    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}

static void test_safe_bundle_publish(void)
{
    catalog_guard guard;
    mutex_context mutex;
    catalog_stage_snapshot active_snapshot, candidate_snapshot;
    int active_items[3] = {1, 2, 3}, candidate_items[2] = {3, 4};
    int active_sidecars[3] = {10, 20, 30}, candidate_sidecars[2] = {30, 40};
    catalog_stage_bundle active, candidate;
    bundle_prepare_context prepare_context;

    memset(&active_snapshot, 0, sizeof(active_snapshot));
    memset(&candidate_snapshot, 0, sizeof(candidate_snapshot));
    set_record(&active_snapshot.records[0], 1, 0, 1, "DVD/Old.iso", 100, 1);
    active_snapshot.count = 1;
    set_record(&candidate_snapshot.records[0], 1, 0, 1, "DVD/Old.iso", 100, 1);
    candidate_snapshot.count = 1;

    active.items = active_items;
    active.items_capacity = 3;
    active.sidecars = active_sidecars;
    active.sidecars_capacity = 3;
    active.count = 1;
    active.selected_index = 0;
    active.snapshot = &active_snapshot;
    candidate.items = candidate_items;
    candidate.items_capacity = 2;
    candidate.sidecars = candidate_sidecars;
    candidate.sidecars_capacity = 2;
    candidate.count = 1;
    candidate.selected_index = -1;
    candidate.snapshot = &candidate_snapshot;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));
    prepare_context.guard = &guard;
    prepare_context.fail = 0;
    prepare_context.calls = 0;

    /* The whole bundle stays unchanged until an explicit safe-point request. */
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(candidate.items == candidate_items && candidate.sidecars == candidate_sidecars);

    assert(catalog_guard_request_apply(&guard));
    assert(!catalog_guard_read_begin(&guard));
    assert(catalog_stage_bundle_publish_prepared(&guard, &active, &candidate,
                                                  migrate_bundle_sidecars, &prepare_context) == CATALOG_STAGE_OK);
    assert(prepare_context.calls == 1);
    assert(active.items == candidate_items && active.sidecars == candidate_sidecars);
    assert(candidate.items == active_items && candidate.sidecars == active_sidecars);
    assert(active.count == 1 && candidate.count == 1);
    assert(active.items_capacity == 2 && active.sidecars_capacity == 2);
    assert(candidate.items_capacity == 3 && candidate.sidecars_capacity == 3);
    assert(active.snapshot == &candidate_snapshot && candidate.snapshot == &active_snapshot);
    assert(active.selected_index == 0 && candidate.selected_index == 0);
    assert(((int *)active.items)[0] == 3 && ((int *)active.sidecars)[0] == 10);
    assert(((int *)candidate.items)[0] == 1 && ((int *)candidate.sidecars)[0] == 10);
    assert(!guard.apply_pending && !guard.applying);

    /* A failed prepare releases the gate and never swaps active storage. */
    prepare_context.fail = 1;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish_prepared(&guard, &active, &candidate,
                                                  migrate_bundle_sidecars, &prepare_context) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(prepare_context.calls == 2);
    assert(active.items == candidate_items && active.sidecars == candidate_sidecars);
    assert(active.selected_index == 0 && candidate.selected_index == 0);
    assert(!guard.apply_pending && !guard.applying);
    prepare_context.fail = 0;

    /* Existing readers prevent the entire bundle from moving; retry after drain. */
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish_prepared(&guard, &active, &candidate,
                                                  migrate_bundle_sidecars, &prepare_context) == CATALOG_STAGE_ERR_BUSY);
    assert(prepare_context.calls == 2); /* preparation never runs while a reader is active */
    assert(active.items == candidate_items && active.sidecars == candidate_sidecars);
    assert(active.snapshot == &candidate_snapshot);
    assert(catalog_guard_read_end(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_OK);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(active.items_capacity == 3 && active.sidecars_capacity == 3);
    assert(active.snapshot == &active_snapshot);

    /* A duplicate candidate is rejected before any part of the active bundle changes. */
    set_record(&candidate_snapshot.records[1], 1, 0, 1, "DVD/Old.iso", 300, 3);
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

    /* Invalid bundle shape cancels a pending request instead of leaving readers blocked. */
    candidate_snapshot.count = 1;
    candidate.count = 2;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(!guard.apply_pending && !guard.applying);
    candidate.count = 1;
    candidate.sidecars = active.sidecars;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(!guard.apply_pending && !guard.applying);
    candidate.sidecars = candidate_sidecars;
    candidate.items = active.items;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_ARGUMENT);
    assert(!guard.apply_pending && !guard.applying);
    candidate.items = candidate_items;
    assert(active.items == active_items && active.sidecars == active_sidecars);

    /* Capacity metadata is part of the bundle contract; undersized buffers never swap. */
    candidate.items_capacity = 0;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_CAPACITY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(!guard.apply_pending && !guard.applying);
    candidate.items_capacity = 2;

    candidate.sidecars_capacity = 0;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_CAPACITY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(!guard.apply_pending && !guard.applying);
    candidate.sidecars_capacity = 2;

    active.items_capacity = 0;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_CAPACITY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(!guard.apply_pending && !guard.applying);
    active.items_capacity = 3;

    active.sidecars_capacity = 0;
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_CAPACITY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(!guard.apply_pending && !guard.applying);
    active.sidecars_capacity = 3;

    /* Gameplay/storage activity prevents a bundle apply. */
    candidate.sidecars = candidate_sidecars;
    assert(catalog_guard_set_activity(&guard, 1, 0));
    assert(!catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish(&guard, &active, &candidate) == CATALOG_STAGE_ERR_BUSY);
    assert(active.items == active_items && active.sidecars == active_sidecars);
    assert(catalog_guard_set_activity(&guard, 0, 0));

    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}


typedef struct {
    catalog_guard *guard;
    int calls;
} typed_bundle_prepare_context;

static int migrate_typed_bundle_sidecars(const catalog_stage_bundle *active,
                                         catalog_stage_bundle *candidate,
                                         void *opaque)
{
    typed_bundle_prepare_context *context = (typed_bundle_prepare_context *)opaque;
    assert(context != NULL && context->guard != NULL);
    assert(context->guard->applying);
    assert(context->guard->readers == 0);
    ++context->calls;
    return catalog_stage_sidecar_set_migrate(
        active->snapshot, candidate->snapshot,
        (const catalog_stage_sidecar_set *)active->sidecars, active->sidecars_capacity,
        (catalog_stage_sidecar_set *)candidate->sidecars, candidate->sidecars_capacity);
}

typedef struct {
    catalog_guard *guard;
    sidecar_release_counts *releases;
    int calls;
} typed_bundle_retire_context;

static int retire_typed_bundle_sidecars(const catalog_stage_bundle *active,
                                        catalog_stage_bundle *retired,
                                        void *opaque)
{
    typed_bundle_retire_context *context = (typed_bundle_retire_context *)opaque;
    assert(context != NULL && context->guard != NULL && context->releases != NULL);
    assert(context->guard->applying && context->guard->apply_pending);
    assert(context->guard->readers == 0);
    /* The reader gate must stay closed while the retired set is inspected and freed. */
    assert(!catalog_guard_read_begin(context->guard));
    ++context->calls;
    return catalog_stage_sidecar_set_retire(
        retired->snapshot, active->snapshot,
        (catalog_stage_sidecar_set *)retired->sidecars, retired->sidecars_capacity,
        (const catalog_stage_sidecar_set *)active->sidecars, active->sidecars_capacity,
        release_vmc_resource, release_icon_resource, release_cover_resource, context->releases);
}

static void test_end_to_end_candidate_apply(void)
{
    mutex_context mutex;
    catalog_guard guard;
    catalog_stage_snapshot active_snapshot, candidate_snapshot;
    catalog_stage_diff diff;
    scan_fixture fixture;
    int active_items[3] = {1, 2, 3};
    int candidate_items[3] = {4, 5, 6};
    catalog_stage_sidecar_set active_sidecars, candidate_sidecars;
    catalog_stage_bundle active, candidate;
    typed_bundle_prepare_context prepare = {&guard, 0};
    sidecar_release_counts releases = {0, 0, 0};
    typed_bundle_retire_context retire = {&guard, &releases, 0};
    void *alpha_vmc, *alpha_icon, *alpha_cover;
    void *beta_vmc, *beta_icon, *beta_cover;

    memset(&active_snapshot, 0, sizeof(active_snapshot));
    memset(&candidate_snapshot, 0, sizeof(candidate_snapshot));
    active_snapshot.count = 3;
    set_record(&active_snapshot.records[0], 1, 0, 1, "DVD/Alpha.iso", 100, 11);
    set_record(&active_snapshot.records[1], 1, 0, 1, "DVD/Beta.iso", 200, 22);
    set_record(&active_snapshot.records[2], 1, 0, 1, "DVD/Removed.iso", 300, 33);

    init_sidecar_set(&active_sidecars);
    init_sidecar_set(&candidate_sidecars);
    alpha_vmc = active_sidecars.vmc_dir[0] = test_resource();
    alpha_icon = active_sidecars.gicon[0] = test_resource();
    alpha_cover = active_sidecars.cover_big[0] = test_resource();
    beta_vmc = active_sidecars.vmc_dir[1] = test_resource();
    beta_icon = active_sidecars.gicon[1] = test_resource();
    beta_cover = active_sidecars.cover_small[1] = test_resource();
    active_sidecars.vmc_dir[2] = test_resource();
    active_sidecars.gicon[2] = test_resource();
    active_sidecars.cover_half[2] = test_resource();
    for (int i = 0; i < 3; ++i) {
        active_sidecars.vmc_state[i] = 2;
        active_sidecars.gicon_state[i] = 2;
    }
    active_sidecars.icon_want = 0;
    active_sidecars.vmc_net_want = 1;
    active_sidecars.ra_want = 2;
    active_sidecars.ra_list_want = 0;
    active_sidecars.ra_list_for = 0;
    active_sidecars.cover_req[0] = 1;
    active_sidecars.cover_done[0] = 4;
    active_sidecars.cover_seen[0] = 5;

    active.items = active_items;
    active.items_capacity = 3;
    active.sidecars = &active_sidecars;
    active.sidecars_capacity = 3;
    active.count = 3;
    active.selected_index = 0; /* Alpha */
    active.snapshot = &active_snapshot;

    candidate.items = candidate_items;
    candidate.items_capacity = 3;
    candidate.sidecars = &candidate_sidecars;
    candidate.sidecars_capacity = 3;
    candidate.count = 0;
    candidate.selected_index = -1;
    candidate.snapshot = &candidate_snapshot;

    assert(pthread_mutex_init(&mutex.mutex, NULL) == 0);
    assert(catalog_guard_init(&guard, guard_lock, guard_unlock, &mutex));

    /* A cancelled scan leaves the published generation and its resources untouched. */
    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = 1;
    fixture.error_at = -1;
    fixture.count = 3;
    set_record(&fixture.records[0], 1, 0, 1, "DVD/Beta.iso", 200, 22);
    set_record(&fixture.records[1], 1, 0, 1, "DVD/Alpha.iso", 100, 11);
    set_record(&fixture.records[2], 1, 0, 1, "DVD/New.iso", 400, 44);
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate_snapshot) ==
           CATALOG_STAGE_SCAN_CANCELLED);
    assert(candidate_snapshot.count == 0);
    assert(!guard.scanning && !guard.apply_pending);
    assert(active.items == active_items && active.sidecars == &active_sidecars);
    assert(active.snapshot == &active_snapshot && active.count == 3);
    assert(active_sidecars.vmc_dir[0] == alpha_vmc && active_sidecars.gicon[0] == alpha_icon);
    assert(active_sidecars.vmc_dir[1] == beta_vmc && active_sidecars.gicon[1] == beta_icon);
    assert(catalog_guard_set_activity(&guard, 0, 0));

    /* Build a complete inactive snapshot, including a reorder, one removal and one addition. */
    memset(&fixture, 0, sizeof(fixture));
    fixture.guard = &guard;
    fixture.cancel_at = -1;
    fixture.error_at = -1;
    fixture.count = 3;
    set_record(&fixture.records[0], 1, 0, 1, "DVD/Beta.iso", 200, 22);
    set_record(&fixture.records[1], 1, 0, 1, "DVD/Alpha.iso", 100, 11);
    set_record(&fixture.records[2], 1, 0, 1, "DVD/New.iso", 400, 44);
    assert(catalog_stage_scan(&guard, scan_fixture_next, &fixture, &candidate_snapshot) ==
           CATALOG_STAGE_SCAN_OK);
    assert(candidate_snapshot.count == 3);
    assert(catalog_stage_snapshot_diff(&active_snapshot, &candidate_snapshot, &diff) ==
           CATALOG_STAGE_OK);
    assert(diff.added_count == 1 && diff.removed_count == 1 && diff.changed_count == 0);
    candidate.count = candidate_snapshot.count;

    /* The real typed sidecar migration runs only after the reader gate reaches the safe point. */
    assert(catalog_guard_request_apply(&guard));
    assert(catalog_stage_bundle_publish_prepared_retire(
               &guard, &active, &candidate,
               migrate_typed_bundle_sidecars, &prepare,
               retire_typed_bundle_sidecars, &retire) == CATALOG_STAGE_OK);
    assert(prepare.calls == 1);
    assert(active.items == candidate_items && active.sidecars == &candidate_sidecars);
    assert(active.snapshot == &candidate_snapshot && active.count == 3);
    assert(active.selected_index == 1); /* Alpha follows its stable key after the reorder. */
    assert(candidate.items == active_items && candidate.sidecars == &active_sidecars);
    assert(candidate.snapshot == &active_snapshot && candidate.count == 3);
    assert(candidate_sidecars.vmc_dir[1] == alpha_vmc && candidate_sidecars.gicon[1] == alpha_icon);
    assert(candidate_sidecars.cover_big[1] == alpha_cover);
    assert(candidate_sidecars.vmc_dir[0] == beta_vmc && candidate_sidecars.gicon[0] == beta_icon);
    assert(candidate_sidecars.cover_small[0] == beta_cover);
    assert(candidate_sidecars.vmc_dir[2] == NULL && candidate_sidecars.gicon[2] == NULL);
    assert(candidate_sidecars.icon_want == 1 && candidate_sidecars.vmc_net_want == 0);
    assert(candidate_sidecars.ra_want == -1);
    assert(candidate_sidecars.ra_list_want == 1 && candidate_sidecars.ra_list_for == 1);
    assert(candidate_sidecars.cover_req[1] == 0);
    assert(candidate_sidecars.cover_done[1] == 4 && candidate_sidecars.cover_seen[1] == 5);
    assert(!guard.apply_pending && !guard.applying);

    /* Retirement ran inside the publisher while the apply gate was still closed. */
    assert(retire.calls == 1);
    assert(releases.vmc_releases == 1 && releases.icon_releases == 1 && releases.cover_releases == 1);
    assert(catalog_guard_read_begin(&guard));
    assert(catalog_guard_read_end(&guard));
    assert(active_sidecars.vmc_dir[2] == NULL && active_sidecars.gicon[2] == NULL);
    assert(active_sidecars.cover_half[2] == NULL);
    assert(candidate_sidecars.vmc_dir[2] == NULL && candidate_sidecars.gicon[2] == NULL);

    free(alpha_vmc);
    free(alpha_icon);
    free(alpha_cover);
    free(beta_vmc);
    free(beta_icon);
    free(beta_cover);
    catalog_stage_sidecar_set_discard(&active_sidecars);
    catalog_stage_sidecar_set_discard(&candidate_sidecars);
    assert(pthread_mutex_destroy(&mutex.mutex) == 0);
}

static void test_disc_slot_fingerprint_resets_only_disc_sidecars(void)
{
    catalog_stage_snapshot old_snapshot, new_snapshot;
    catalog_stage_diff diff;
    sidecar_release_counts releases = {0, 0, 0};
    memset(&old_snapshot, 0, sizeof(old_snapshot));
    memset(&new_snapshot, 0, sizeof(new_snapshot));
    init_sidecar_set(&sidecar_old);
    init_sidecar_set(&sidecar_new);

    old_snapshot.count = 2;
    set_record(&old_snapshot.records[0], 0xffu, 0, 3, "DISC", 0, 1); /* no disc */
    set_record(&old_snapshot.records[1], 1, 0, 0, "DVD/Game.iso", 100, 11);
    new_snapshot.count = 2;
    set_record(&new_snapshot.records[0], 0xffu, 0, 3, "DISC", 0, 2); /* disc inserted/changed */
    set_record(&new_snapshot.records[1], 1, 0, 0, "DVD/Game.iso", 100, 11);

    assert(catalog_stage_snapshot_diff(&old_snapshot, &new_snapshot, &diff) == CATALOG_STAGE_OK);
    assert(diff.changed_count == 1 && diff.changed_old[0] == 0 && diff.changed_new[0] == 0);

    sidecar_old.cover_big[0] = test_resource();
    sidecar_old.cover_small[0] = test_resource();
    sidecar_old.cover_half[0] = test_resource();
    sidecar_old.gicon[0] = test_resource();
    sidecar_old.gicon_state[0] = 2;
    sidecar_old.cover_big[1] = test_resource();
    sidecar_old.gicon[1] = test_resource();
    sidecar_old.gicon_state[1] = 2;

    assert(catalog_stage_sidecar_set_migrate(&old_snapshot, &new_snapshot,
                                              &sidecar_old, 2, &sidecar_new, 2) == CATALOG_STAGE_OK);
    assert(sidecar_new.cover_big[0] == NULL && sidecar_new.gicon[0] == NULL);
    assert(sidecar_new.cover_big[1] == sidecar_old.cover_big[1]);
    assert(sidecar_new.gicon[1] == sidecar_old.gicon[1]);
    assert(catalog_stage_sidecar_set_retire(&old_snapshot, &new_snapshot,
                                             &sidecar_old, 2, &sidecar_new, 2,
                                             release_vmc_resource, release_icon_resource,
                                             release_cover_resource, &releases) == CATALOG_STAGE_OK);
    assert(releases.cover_releases == 3 && releases.icon_releases == 1);
    assert(sidecar_old.cover_big[0] == NULL && sidecar_old.gicon[0] == NULL);
    /* The unchanged game's resources now belong to the new generation. */
    free(sidecar_new.cover_big[1]);
    free(sidecar_new.gicon[1]);
    sidecar_new.cover_big[1] = NULL;
    sidecar_new.gicon[1] = NULL;
    catalog_stage_sidecar_set_discard(&sidecar_new);
}

int main(void)
{
    test_staged_scan_builder();
    test_concurrent_scan_cancellation();
    test_safe_snapshot_publish();
    test_safe_bundle_publish();
    test_end_to_end_candidate_apply();
    test_bundle_selection_rebase();
    test_path_and_validation();
    test_snapshot_append();
    test_snapshot_index_map();
    test_sidecar_migration();
    test_typed_sidecar_set_migration_and_retirement();
    test_typed_sidecar_set_discard_and_fail_closed();
    test_typed_sidecar_set_rejects_aliases_and_unexpected_borrows();
    test_disc_slot_fingerprint_resets_only_disc_sidecars();
    test_diff_detects_large_file_size_changes();
    test_diff_and_stable_selection();
    test_guard_and_concurrent_reader();
    test_download_gate();
    puts("catalog stage: staged scan, end-to-end candidate/apply, typed sidecar retirement, append, map, bundle selection rebase, path, validation, diff, selection, safe snapshot publish, concurrent apply and download gates PASS");
    return 0;
}
