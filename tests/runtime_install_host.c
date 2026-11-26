#define _POSIX_C_SOURCE 200809L
#include "runtime_install.h"
#include "runtime_payload.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const unsigned char payload[] = "123456789";
static const unsigned int payload_size = 9;
const tea_runtime_asset tea_runtime_assets[] = {
    { "OPL.ELF", payload, &payload_size, 0xcbf43926u }
};
const unsigned int tea_runtime_assets_count = 1;
const char tea_runtime_bundle_id[] = "selftest-v1";

static void write_text(const char *path, const char *value)
{
    FILE *f = fopen(path, "wb");
    assert(f);
    assert(fwrite(value, 1, strlen(value), f) == strlen(value));
    assert(fclose(f) == 0);
}

static void assert_text(const char *path, const char *expected)
{
    char actual[64] = {0};
    FILE *f = fopen(path, "rb");
    assert(f);
    assert(fread(actual, 1, sizeof(actual) - 1, f) == strlen(expected));
    assert(fclose(f) == 0);
    assert(strcmp(actual, expected) == 0);
}

int main(void)
{
    char root_template[] = "/tmp/tea_g3_runtime_install_selftest.XXXXXX";
    char *root = mkdtemp(root_template);
    char asset_tmp[160], marker_tmp[160], asset_path[160], marker_path[160];
    struct stat st;

    assert(root);
    snprintf(asset_tmp, sizeof(asset_tmp), "%s/OPL.ELF.tmp", root);
    snprintf(marker_tmp, sizeof(marker_tmp), "%s/.bundle-id.tmp", root);
    snprintf(asset_path, sizeof(asset_path), "%s/OPL.ELF", root);
    snprintf(marker_path, sizeof(marker_path), "%s/.bundle-id", root);

    /* A stale/user-owned asset staging file must not be truncated or removed. */
    write_text(asset_tmp, "KEEP-ASSET-TMP");
    assert(runtime_install(root) == 0);
    assert_text(asset_tmp, "KEEP-ASSET-TMP");
    assert(stat(asset_path, &st) != 0);
    assert(remove(asset_tmp) == 0);

    /* The bundle marker uses the same exclusive-create rule. */
    write_text(marker_tmp, "KEEP-MARKER-TMP");
    assert(runtime_install(root) == 0);
    assert_text(marker_tmp, "KEEP-MARKER-TMP");
    assert_text(asset_path, "123456789");
    assert(stat(marker_path, &st) != 0);
    assert(remove(marker_tmp) == 0);

    assert(runtime_install(root) == 1);
    assert(runtime_asset_valid(root, "OPL.ELF") == 1);
    assert_text(asset_path, "123456789");

    assert(remove(asset_path) == 0);
    assert(remove(marker_path) == 0);
    assert(rmdir(root) == 0);
    puts("runtime installer preserves pre-existing staging files");
    return 0;
}
