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
    if (!ok || rename(tmp, path) != 0) { remove(tmp); return 0; }
    return 1;
}

int runtime_install(const char *home)
{
    char root[128], marker[144], expected[32], actual[32];
    if (snprintf(root, sizeof(root), "%s/tea_g3/runtime", home) >= (int)sizeof(root) ||
        snprintf(marker, sizeof(marker), "%s/.bundle-id", root) >= (int)sizeof(marker))
        return 0;
    if (!make_parents(root)) return 0;
    snprintf(expected, sizeof(expected), "%s", tea_runtime_bundle_id);
    FILE *f = fopen(marker, "rb");
    if (f) {
        size_t n = fread(actual, 1, sizeof(actual) - 1, f);
        actual[n] = 0;
        fclose(f);
        if (!strcmp(actual, expected)) return 1;
    }
    for (unsigned int i = 0; i < tea_runtime_assets_count; i++) {
        if (!write_asset(root, &tea_runtime_assets[i])) {
            printf("runtime: failed to install %s\n", tea_runtime_assets[i].path);
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
    printf("runtime: installed bundle %s (%u files)\n", expected, tea_runtime_assets_count);
    return 1;
}
