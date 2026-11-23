#pragma once
#include <tamtypes.h>
#define FIO_SEEK_SET 0
#define FIO_SEEK_END 2
s64 fileXioLseek64(int fd, s64 offset, int whence);
int fileXioRead(int fd, void *buffer, int size);
