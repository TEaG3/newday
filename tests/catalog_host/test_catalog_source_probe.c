#define _POSIX_C_SOURCE 200809L

#include "catalog_source_probe.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    char temporary[] = "/tmp/teag3-source-root-XXXXXX";
    char missing[256];
    char regular_file[256];
    char long_root[512];
    char *root = mkdtemp(temporary);
    FILE *file;

    assert(root != NULL);
    assert(catalog_source_root_probe(root, 0));
    assert(catalog_source_root_probe(root, 1));

    assert(snprintf(missing, sizeof(missing), "%s/missing", root) > 0);
    assert(!catalog_source_root_probe(missing, 0));

    assert(snprintf(regular_file, sizeof(regular_file), "%s/not-a-directory", root) > 0);
    file = fopen(regular_file, "wb");
    assert(file != NULL);
    assert(fputs("file", file) >= 0);
    assert(fclose(file) == 0);
    assert(!catalog_source_root_probe(regular_file, 0));
    assert(unlink(regular_file) == 0);

    memset(long_root, 'x', sizeof(long_root));
    long_root[sizeof(long_root) - 1] = '\0';
    assert(!catalog_source_root_probe(long_root, 0));
    assert(!catalog_source_root_probe(NULL, 0));
    assert(!catalog_source_root_probe("", 0));

    assert(rmdir(root) == 0);
    puts("catalog source-root probe: PASS (mounted, missing, non-directory, bounds)");
    return 0;
}
