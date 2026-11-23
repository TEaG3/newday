#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zso.h"

#define DATA_OFFSET (sizeof(ZISO_header) + ZISO_IDX_MAX_ENTRIES * sizeof(u32))
#define IMAGE_SIZE (DATA_OFFSET + 4096)
static u8 image[IMAGE_SIZE];
static int fail_alloc;
static int fail_read;

void *ziso_alloc(u32 size) { return fail_alloc ? NULL : malloc(size); }

int read_raw_data(u8 *dst, u32 size, u32 offset, u32 shift)
{
    uint64_t start = (uint64_t)offset << shift;
    if (fail_read || start + size > sizeof(image)) return -1;
    memcpy(dst, image + start, size);
    return 0;
}

static void reset_fixture(ZISO_header *header)
{
    memset(image, 0, sizeof(image));
    memset(header, 0, sizeof(*header));
    header->magic = ZSO_MAGIC;
    header->header_size = sizeof(*header);
    header->total_bytes = 4096;
    header->block_size = 2048;
    memcpy(image, header, sizeof(*header));
    ziso_idx_start_block = -1;
    ziso_total_block = 0;
}

int main(void)
{
    ZISO_header header;
    u8 output[4096], raw[2048];
    u32 *index = (u32 *)(image + sizeof(ZISO_header));
    static const u8 compressed_block[] = {
        0x1f, 'A', 0x01, 0x00, 255, 255, 255, 255, 255, 255, 255, 243
    };

    reset_fixture(&header);
    for (u32 i = 0; i < 2048; ++i) raw[i] = (u8)(i & 0xff);
    index[0] = DATA_OFFSET | 0x80000000u;
    index[1] = DATA_OFFSET + 2048;
    index[2] = DATA_OFFSET + 2048 + sizeof(compressed_block);
    memcpy(image + DATA_OFFSET, raw, sizeof(raw));
    memcpy(image + DATA_OFFSET + 2048, compressed_block, sizeof(compressed_block));

    ziso_init(&header, 0);
    assert(ziso_total_block == 2);
    assert(ziso_read_sector(output, 0, 2) == 2);
    assert(memcmp(output, raw, 2048) == 0);
    for (u32 i = 0; i < 2048; ++i) assert(output[2048 + i] == 'A');
    assert(ziso_read_sector(output, 2, 1) == 0);
    assert(ziso_read_sector(output, 0, 0) == 0);

    /* Malformed LZ4 stream is rejected. */
    index[1] = DATA_OFFSET + 2048;
    index[2] = DATA_OFFSET + 2049;
    image[DATA_OFFSET + 2048] = 0xff;
    ziso_idx_start_block = -1;
    assert(ziso_read_sector(output, 1, 1) == 0);

    /* Index/data I/O failures are reported. */
    ziso_idx_start_block = -1;
    fail_read = 1;
    assert(ziso_read_sector(output, 0, 1) == 0);
    fail_read = 0;

    /* Allocation failure leaves the decoder disabled. */
    ziso_tmp_buf = NULL;
    ziso_idx_cache = NULL;
    ziso_idx_start_block = -1;
    fail_alloc = 1;
    ziso_init(&header, 0);
    assert(ziso_read_sector(output, 0, 1) == 0);
    fail_alloc = 0;

    puts("ZSO host fixture: PASS (raw + compressed + bounds + I/O/allocation failures)");
    return 0;
}
