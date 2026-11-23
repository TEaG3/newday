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

void ziso_init(ZISO_header *header, u32 first_block)
{
    (void)first_block;
    ziso_idx_start_block = -1;
    ziso_total_block = 0;

    if (!header || header->magic != ZSO_MAGIC || header->block_size != 2048 ||
        header->align >= 32) {
        return;
    }

    ziso_align = header->align;
    u32 *total_bytes_p = (u32 *)&header->total_bytes;
    ziso_total_block = (total_bytes_p[0] >> 11) |
                       ((total_bytes_p[1] & 0x7ff) << 21);

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

/*
 * Read sectors from a ZSO image. Index offsets are stored in units of
 * 1 << ziso_align. Compressed blocks are decoded with the bounded LZ4 API.
 */
int ziso_read_sector(u8 *addr, u32 lsn, unsigned int count)
{
    if (!addr || count == 0 || !ziso_tmp_buf || !ziso_idx_cache ||
        lsn >= ziso_total_block) {
        return 0;
    }

    if (count > ziso_total_block - lsn) {
        count = ziso_total_block - lsn;
    }
    if (count > ZISO_IDX_MAX_ENTRIES - 1) {
        count = ZISO_IDX_MAX_ENTRIES - 1;
    }

    if (ziso_idx_start_block < 0 || lsn < (u32)ziso_idx_start_block ||
        lsn + count > (u32)ziso_idx_start_block + ZISO_IDX_MAX_ENTRIES - 1) {
        u64 index_offset = (u64)sizeof(ZISO_header) + (u64)lsn * sizeof(u32);
        u32 index_bytes = (count + 1) * sizeof(u32);
        if (index_offset > 0xffffffffu ||
            read_raw_data((u8 *)ziso_idx_cache, index_bytes, (u32)index_offset, 0) != 0) {
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

        if (end_units <= start_units ||
            start_units > (0xffffffffu >> ziso_align) ||
            (end_units - start_units) > (0xffffffffu >> ziso_align)) {
            return (int)i;
        }

        u32 data_offset = start_units << ziso_align;
        u32 stored_size = (end_units - start_units) << ziso_align;
        u8 *sector_out = addr + (i * 2048);

        if (start_entry & 0x80000000u) {
            if (stored_size != 2048 ||
                read_raw_data(sector_out, 2048, data_offset, 0) != 0) {
                return (int)i;
            }
        } else {
            if (stored_size == 0 || stored_size > 2048 ||
                read_raw_data(ziso_tmp_buf, stored_size, data_offset, 0) != 0) {
                return (int)i;
            }
            int decoded = LZ4_decompress_safe((const char *)ziso_tmp_buf,
                                              (char *)sector_out,
                                              (int)stored_size, 2048);
            if (decoded != 2048) {
                return (int)i;
            }
        }
    }

    return (int)count;
}
