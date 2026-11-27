#ifndef TEAG3_TEST_IO_COMMON_H
#define TEAG3_TEST_IO_COMMON_H
#include <tamtypes.h>
typedef struct {
    char name[64];
    struct {
        u32 mode;
        u32 attr;
        u64 size;
        u32 private_5;
    } stat;
} iox_dirent_t;
#endif
