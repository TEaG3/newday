#include "catalog_source_probe.h"

#include <dirent.h>
#include <stdio.h>

#define CATALOG_SOURCE_ROOT_PATH_MAX 256

int catalog_source_root_probe(const char *root, int no_trailing_slash)
{
    char path[CATALOG_SOURCE_ROOT_PATH_MAX];
    DIR *directory;
    int length;

    if (root == NULL || root[0] == '\0') return 0;
    length = snprintf(path, sizeof(path), "%s%s", root,
                      no_trailing_slash ? "" : "/");
    if (length < 0 || (size_t)length >= sizeof(path)) return 0;

    directory = opendir(path);
    if (directory == NULL) return 0;
    return closedir(directory) == 0;
}
