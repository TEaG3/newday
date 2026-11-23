// EE-side ZSO reader used only while TEa.G3 scans a .zso file for SYSTEM.CNF.
// Neutrino has a separate copy of the same decoder for the actual game-sector path.
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>

#include "third_party/opl-zso/zso.c"

static int zso_ee_fd = -1;
static u64 zso_ee_file_bytes;

void *ziso_alloc(u32 size)
{
    return memalign(64, size);
}

int read_raw_data(u8 *addr, u32 size, u32 offset, u32 shift)
{
    u64 start;

    if (zso_ee_fd < 0 || addr == NULL || shift >= 32 || size > 0x7fffffffu)
        return -1;

    start = (u64)offset << shift;
    if (start > zso_ee_file_bytes || (u64)size > zso_ee_file_bytes - start)
        return -1;

    if (fileXioLseek64(zso_ee_fd, (s64)start, FIO_SEEK_SET) < 0)
        return -1;

    return fileXioRead(zso_ee_fd, addr, (int)size) == (int)size ? 0 : -1;
}

int zso_ee_open(int fd)
{
    ZISO_header header;
    s64 end;

    zso_ee_fd = fd;
    zso_ee_file_bytes = 0;
    end = fileXioLseek64(fd, 0, FIO_SEEK_END);
    if (end < 0)
        return -1;
    zso_ee_file_bytes = (u64)end;

    if (read_raw_data((u8 *)&header, sizeof(header), 0, 0) != 0 ||
        header.magic != ZSO_MAGIC)
        return -1;

    ziso_init(&header, 0);
    if (ziso_total_block == 0 || ziso_idx_cache == NULL || ziso_tmp_buf == NULL)
        return -1;

    return 1;
}

int zso_ee_read(unsigned lba, void *buf, unsigned sectors)
{
    return zso_ee_fd >= 0 && ziso_read_sector((u8 *)buf, lba, sectors) == (int)sectors;
}

void zso_ee_close(void)
{
    zso_ee_fd = -1;
    zso_ee_file_bytes = 0;
}
