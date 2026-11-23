#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "zso_ee.h"
#include "third_party/opl-zso/zso.h"

#define TEST_HEADER_SIZE 32
#define DATA_OFFSET (TEST_HEADER_SIZE + ZISO_IDX_MAX_ENTRIES * sizeof(u32))
#define TEST_FILE_SIZE (DATA_OFFSET + 2048 + 32)

static u8 disk[TEST_FILE_SIZE];
static u64 disk_size;
static s64 disk_pos;

s64 fileXioLseek64(int fd, s64 offset, int whence)
{
    if (fd != 7) return -1;
    if (whence == FIO_SEEK_END) disk_pos = (s64)disk_size + offset;
    else if (whence == FIO_SEEK_SET) disk_pos = offset;
    else return -1;
    return disk_pos < 0 ? -1 : disk_pos;
}

int fileXioRead(int fd, void *buffer, int size)
{
    if (fd != 7 || disk_pos < 0 || (u64)disk_pos > disk_size ||
        (u64)size > disk_size - (u64)disk_pos)
        return -1;
    memcpy(buffer, disk + disk_pos, size);
    disk_pos += size;
    return size;
}

static void make_zso(void)
{
    ZISO_header h;
    u32 *idx;
    static const u8 compressed[] = {
        0x1f, 'A', 0x01, 0x00, 255, 255, 255, 255, 255, 255, 255, 243
    };

    memset(disk, 0, sizeof(disk));
    memset(&h, 0, sizeof(h));
    h.magic = ZSO_MAGIC;
    h.header_size = TEST_HEADER_SIZE;
    h.total_bytes = 4096;
    h.block_size = 2048;
    memcpy(disk, &h, sizeof(h));

    idx = (u32 *)(disk + h.header_size);
    idx[0] = DATA_OFFSET | 0x80000000u;
    idx[1] = DATA_OFFSET + 2048;
    idx[2] = DATA_OFFSET + 2048 + sizeof(compressed);
    for (u32 i = 0; i < 2048; ++i) disk[DATA_OFFSET + i] = (u8)(i & 0xff);
    memcpy(disk + DATA_OFFSET + 2048, compressed, sizeof(compressed));
    disk_size = DATA_OFFSET + 2048 + sizeof(compressed);
    disk_pos = 0;
}

int main(void)
{
    u8 out[4096];

    make_zso();
    assert(zso_ee_open(7) == 1);
    assert(zso_ee_read(0, out, 2));
    for (u32 i = 0; i < 2048; ++i) {
        assert(out[i] == (u8)(i & 0xff));
        assert(out[2048 + i] == 'A');
    }
    assert(!zso_ee_read(2, out, 1));

    /* The storage adapter must reject truncated compressed payloads. */
    disk_size = DATA_OFFSET + 2048 + 1;
    assert(!zso_ee_read(1, out, 1));

    /* A recognized but malformed ZSO header must not be treated as an ISO. */
    make_zso();
    ((ZISO_header *)disk)->header_size = sizeof(ZISO_header) - 1;
    assert(zso_ee_open(7) == -1);

    /* Ordinary ISO files are left to the original ISO reader. */
    memset(disk, 0, sizeof(disk));
    memcpy(disk + 2048, "CD001", 5);
    disk_size = sizeof(disk);
    assert(zso_ee_open(7) == 0);

    zso_ee_close();
    puts("ZSO EE scanner adapter: PASS (decode + bounds + malformed + ISO fallback)");
    return 0;
}
