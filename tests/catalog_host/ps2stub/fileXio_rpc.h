#ifndef TEAG3_TEST_FILEXIO_RPC_H
#define TEAG3_TEST_FILEXIO_RPC_H
#include <io_common.h>
int fileXioDopen(const char *path);
int fileXioDread(int fd, iox_dirent_t *entry);
int fileXioDclose(int fd);
int fileXioDevctl(const char *device, int command, void *arg, int arg_size, void *buffer, int buffer_size);
#endif
