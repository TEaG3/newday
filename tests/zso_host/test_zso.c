#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zso.h"

#define IMAGE_SIZE (sizeof(ZISO_header) + ZISO_IDX_MAX_ENTRIES * sizeof(u32) + 2048)
static u8 image[IMAGE_SIZE];

void *ziso_alloc(u32 size) { return malloc(size); }

int read_raw_data(u8 *dst, u32 size, u32 offset, u32 shift)
{
    uint64_t start = (uint64_t)offset << shift;
    if (start + size > sizeof(image)) return -1;
    memcpy(dst, image + start, size);
    return 0;
}

int main(void)
{
    ZISO_header header;
    memset(&header, 0, sizeof(header));
    header.magic = ZSO_MAGIC;
    header.header_size = sizeof(header);
    header.total_bytes = 2048;
    header.block_size = 2048;
    header.align = 0;

    const u32 data_offset = sizeof(header) + ZISO_IDX_MAX_ENTRIES * sizeof(u32);
    memcpy(image, &header, sizeof(header));
    u32 *index = (u32 *)(image + sizeof(header));
    index[0] = data_offset | 0x80000000u;       /* uncompressed block */
    index[1] = data_offset + 2048;              /* end offset */
    for (u32 i = 0; i < 2048; ++i) image[data_offset + i] = (u8)(i & 0xff);

    ziso_init(&header, 0);
    assert(ziso_total_block == 1);
    u8 output[2048];
    memset(output, 0, sizeof(output));
    assert(ziso_read_sector(output, 0, 1) == 1);
    assert(memcmp(output, image + data_offset, sizeof(output)) == 0);
    assert(ziso_read_sector(output, 1, 1) == 0);
    puts("ZSO host fixture: PASS (single raw sector and out-of-range read)");
    return 0;
}
