#include "runtime_install.h"
#include "runtime_payload.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static int make_parents(char *path)
{
    char *colon = strchr(path, ':');
    char *p = colon ? colon + 2 : path;
    for (; *p; p++) {
        if (*p != '/') continue;
        *p = 0;
        if (*path && mkdir(path, 0777) < 0) {
            DIR *d = opendir(path);
            if (!d) { *p = '/'; return 0; }
            closedir(d);
        }
        *p = '/';
    }
    if (*path && mkdir(path, 0777) < 0) {
        DIR *d = opendir(path);
        if (!d) return 0;
        closedir(d);
    }
    return 1;
}

static unsigned int crc32_update(unsigned int crc, const unsigned char *data, size_t size)
{
    while (size--) {
        crc ^= *data++;
        for (unsigned int bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc;
}

static int file_matches(const char *path, const tea_runtime_asset *asset)
{
    unsigned char buffer[4096];
    unsigned int crc = 0xFFFFFFFFu;
    unsigned long total = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    for (;;) {
        size_t n = fread(buffer, 1, sizeof(buffer), f);
        if (n) {
            crc = crc32_update(crc, buffer, n);
            total += (unsigned long)n;
        }
        if (n < sizeof(buffer)) {
            if (ferror(f)) { fclose(f); return 0; }
            break;
        }
    }
    if (fclose(f) != 0) return 0;
    crc ^= 0xFFFFFFFFu;
    return total == (unsigned long)*asset->size && crc == asset->crc32;
}

static int write_asset(const char *root, const tea_runtime_asset *asset)
{
    char path[256], tmp[264];
    if (snprintf(path, sizeof(path), "%s/%s", root, asset->path) >= (int)sizeof(path) ||
        snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp))
        return 0;
    char parent[256];
    snprintf(parent, sizeof(parent), "%s", path);
    char *slash = strrchr(parent, '/');
    if (!slash) return 0;
    *slash = 0;
    if (!make_parents(parent)) return 0;
    FILE *f = fopen(tmp, "wb");
    if (!f) return 0;
    int ok = fwrite(asset->data, 1, *asset->size, f) == *asset->size;
    if (fclose(f) != 0) ok = 0;
    if (!ok || !file_matches(tmp, asset)) { remove(tmp); return 0; }
    remove(path);
    if (rename(tmp, path) != 0) { remove(tmp); return 0; }
    return file_matches(path, asset);
}

static int asset_present(const char *root, const tea_runtime_asset *asset)
{
    char path[256];
    if (snprintf(path, sizeof(path), "%s/%s", root, asset->path) >= (int)sizeof(path)) return 0;
    return file_matches(path, asset);
}

int runtime_install(const char *home)
{
    char root[128], marker[144], expected[32], actual[32];
    if (snprintf(root, sizeof(root), "%s", home) >= (int)sizeof(root) ||
        snprintf(marker, sizeof(marker), "%s/.bundle-id", root) >= (int)sizeof(marker))
        return 0;
    if (!make_parents(root)) return 0;
    snprintf(expected, sizeof(expected), "%s", tea_runtime_bundle_id);
    FILE *f = fopen(marker, "rb");
    if (f) {
        size_t n = fread(actual, 1, sizeof(actual) - 1, f);
        actual[n] = 0;
        fclose(f);
        if (n && actual[n - 1] == 10) actual[n - 1] = 0;
        if (!strcmp(actual, expected)) {
            int complete = 1;
            for (unsigned int i = 0; i < tea_runtime_assets_count; i++)
                if (!asset_present(root, &tea_runtime_assets[i])) { complete = 0; break; }
            if (complete) return 1;
        }
    }
    for (unsigned int i = 0; i < tea_runtime_assets_count; i++) {
        if (!write_asset(root, &tea_runtime_assets[i])) {
            printf("runtime: failed integrity check/install for %s\n", tea_runtime_assets[i].path);
            return 0;
        }
    }
    char tmp[152];
    snprintf(tmp, sizeof(tmp), "%s.tmp", marker);
    f = fopen(tmp, "wb");
    if (!f) return 0;
    int ok = fprintf(f, "%s\n", expected) > 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok || rename(tmp, marker) != 0) { remove(tmp); return 0; }
    printf("runtime: installed and verified bundle %s (%u files)\n", expected, tea_runtime_assets_count);
    return 1;
}
