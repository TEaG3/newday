#include "lz4.h"
#include "zso.h"
#include <stdint.h>

#ifdef _IOP
#include <sysclib.h>
#endif

u32 *ziso_idx_cache = NULL;
int ziso_idx_start_block = -1;
u32 ziso_align;
u32 ziso_total_block;
u8 *ziso_tmp_buf = NULL;
static u32 ziso_header_size;

/* Bounded LZ4 block decoder: never reads past input or writes past output. */
static int zso_lz4_decode(const u8 *src, u32 src_size, u8 *dst, u32 dst_size)
{
    const u8 *ip = src, *iend = src + src_size;
    u8 *op = dst, *oend = dst + dst_size;

    while (ip < iend) {
        u32 token = *ip++;
        u32 literals = token >> 4;
        if (literals == 15) {
            u8 extension;
            do {
                if (ip >= iend) return -1;
                extension = *ip++;
                if (literals > 0xffffffffu - extension) return -1;
                literals += extension;
            } while (extension == 255);
        }
        if ((u32)(iend - ip) < literals || (u32)(oend - op) < literals) return -1;
        for (u32 i = 0; i < literals; ++i) *op++ = *ip++;

        if (ip == iend) return (op == oend) ? (int)dst_size : -1;
        if ((u32)(iend - ip) < 2) return -1;
        u32 offset = ip[0] | ((u32)ip[1] << 8);
        ip += 2;
        if (offset == 0 || offset > (u32)(op - dst)) return -1;

        u32 match = (token & 0x0fu) + 4;
        if ((token & 0x0fu) == 15) {
            u8 extension;
            do {
                if (ip >= iend) return -1;
                extension = *ip++;
                if (match > 0xffffffffu - extension) return -1;
                match += extension;
            } while (extension == 255);
        }
        if ((u32)(oend - op) < match) return -1;
        while (match--) {
            *op = *(op - offset);
            ++op;
        }
    }
    return (op == oend) ? (int)dst_size : -1;
}

static int zso_total_bytes_to_blocks(const ZISO_header *header, u32 *blocks)
{
    const u32 *total = (const u32 *)&header->total_bytes;
    u32 low = total[0];
    u32 high = total[1];

    /* PS2 IOP sector offsets are 32-bit. Decode the 64-bit byte count as
       two words and reject values requiring more than 32-bit logical LBAs. */
    if ((low & 2047u) != 0 || high > 0x7ffu)
        return 0;

    *blocks = (high << 21) | (low >> 11);
    return *blocks != 0;
}

void ziso_init(ZISO_header *header, u32 first_block)
{
    u32 total_blocks;
    u32 index_capacity;
    (void)first_block;
    ziso_idx_start_block = -1;
    ziso_total_block = 0;
    ziso_header_size = 0;
    ziso_align = 0;
    if (!header || header->magic != ZSO_MAGIC ||
        header->header_size < sizeof(ZISO_header) || header->header_size > 4096 ||
        header->block_size != 2048 || header->align >= 32 ||
        !zso_total_bytes_to_blocks(header, &total_blocks)) return;

    index_capacity = (0xffffffffu - header->header_size) / sizeof(u32);
    if (total_blocks >= index_capacity) return;

    ziso_align = header->align;
    ziso_header_size = header->header_size;
    ziso_total_block = total_blocks;

    if (!ziso_tmp_buf) {
        u8 *allocation = ziso_alloc(2048 + sizeof(u32) * ZISO_IDX_MAX_ENTRIES + 63);
        if (!allocation) {
            ziso_idx_cache = NULL;
            ziso_total_block = 0;
            return;
        }
        uintptr_t aligned = ((uintptr_t)allocation + 63u) & ~(uintptr_t)63u;
        ziso_tmp_buf = (u8 *)aligned;
        ziso_idx_cache = (u32 *)(ziso_tmp_buf + 2048);
    }
}

int ziso_read_sector(u8 *addr, u32 lsn, unsigned int count)
{
    if (!addr || count == 0 || !ziso_tmp_buf || !ziso_idx_cache ||
        lsn >= ziso_total_block) return 0;

    if (count > ziso_total_block - lsn) count = ziso_total_block - lsn;
    if (count > ZISO_IDX_MAX_ENTRIES - 1) count = ZISO_IDX_MAX_ENTRIES - 1;

    u32 index_offset = ziso_header_size + lsn * sizeof(u32);
    u32 index_end = ziso_header_size + (ziso_total_block + 1) * sizeof(u32);
    u32 align_mask = ((u32)1 << ziso_align) - 1u;
    u32 min_data_units = index_end >> ziso_align;
    if (index_end & align_mask) min_data_units++;
    {
        u32 index_bytes = (count + 1) * sizeof(u32);
        if (read_raw_data((u8 *)ziso_idx_cache, index_bytes, index_offset, 0) != 0) {
            ziso_idx_start_block = -1;
            return 0;
        }
        ziso_idx_start_block = (int)lsn;
    }

    for (unsigned int i = 0; i < count; ++i) {
        u32 start_entry = ziso_idx_cache[i];
        u32 end_entry = ziso_idx_cache[i + 1];
        u32 start_units = start_entry & 0x7fffffffu;
        u32 end_units = end_entry & 0x7fffffffu;
        if (end_units <= start_units || start_units < min_data_units ||
            start_units > (0xffffffffu >> ziso_align) ||
            (end_units - start_units) > (0xffffffffu >> ziso_align)) return (int)i;

        u32 data_offset = start_units << ziso_align;
        u32 stored_size = (end_units - start_units) << ziso_align;
        u8 *sector_out = addr + (i * 2048);

        if (start_entry & 0x80000000u) {
            if (stored_size != 2048 ||
                read_raw_data(sector_out, 2048, data_offset, 0) != 0) return (int)i;
        } else {
            if (stored_size == 0 || stored_size > 2048 ||
                read_raw_data(ziso_tmp_buf, stored_size, data_offset, 0) != 0 ||
                zso_lz4_decode(ziso_tmp_buf, stored_size, sector_out, 2048) != 2048) return (int)i;
        }
    }
    return (int)count;
}
