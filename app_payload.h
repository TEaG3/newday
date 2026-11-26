#ifndef TEAG3_APP_PAYLOAD_H
#define TEAG3_APP_PAYLOAD_H

#include <stddef.h>

#define APP_PAYLOAD_ELF_HEADER_SIZE 52u
#define APP_PAYLOAD_ELF_PHDR_SIZE 32u
#define APP_PAYLOAD_USER_RAM_BASE 0x00100000u
#define APP_PAYLOAD_PT_LOAD 1u
#define APP_PAYLOAD_PF_X 1u

static inline unsigned app_payload_u16(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8;
}

static inline unsigned app_payload_u32(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 |
           (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/*
 * Validate the complete loadable ELF32/MIPS image before writing it to disk.
 * PT_LOAD file ranges must fit in the received body and virtual ranges must
 * stay inside EE user RAM, above the kernel/second-stage loader reservation.
 */
static inline int app_payload_is_valid(const unsigned char *data, size_t length,
                                       unsigned expected_size, unsigned memory_limit)
{
    unsigned entry, phoff, phnum;
    unsigned load_segments = 0, entry_loaded = 0;

    if (data == NULL || expected_size == 0 ||
        length != (size_t)expected_size || length < APP_PAYLOAD_ELF_HEADER_SIZE)
        return 0;
    if (data[0] != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F' ||
        data[4] != 1 || data[5] != 1 || data[6] != 1)
        return 0; /* ELF32, little endian, current version */
    if (app_payload_u16(data + 16) != 2 || app_payload_u16(data + 18) != 8 ||
        app_payload_u32(data + 20) != 1)
        return 0; /* ET_EXEC, EM_MIPS, ELF header version */
    if (app_payload_u16(data + 40) != APP_PAYLOAD_ELF_HEADER_SIZE ||
        app_payload_u16(data + 42) != APP_PAYLOAD_ELF_PHDR_SIZE)
        return 0;
    if (memory_limit <= APP_PAYLOAD_USER_RAM_BASE)
        return 0;

    phoff = app_payload_u32(data + 28);
    phnum = app_payload_u16(data + 44);
    if (phoff < APP_PAYLOAD_ELF_HEADER_SIZE || phoff > length || !phnum ||
        (size_t)phnum > (length - phoff) / APP_PAYLOAD_ELF_PHDR_SIZE)
        return 0;

    entry = app_payload_u32(data + 24);
    for (unsigned i = 0; i < phnum; i++) {
        const unsigned char *ph = data + phoff + (size_t)i * APP_PAYLOAD_ELF_PHDR_SIZE;
        unsigned type = app_payload_u32(ph);
        unsigned offset, vaddr, filesz, memsz, flags, align;

        if (type != APP_PAYLOAD_PT_LOAD)
            continue;
        offset = app_payload_u32(ph + 4);
        vaddr = app_payload_u32(ph + 8);
        filesz = app_payload_u32(ph + 16);
        memsz = app_payload_u32(ph + 20);
        flags = app_payload_u32(ph + 24);
        align = app_payload_u32(ph + 28);

        if (!memsz || filesz > memsz ||
            vaddr < APP_PAYLOAD_USER_RAM_BASE || vaddr >= memory_limit ||
            memsz > memory_limit - vaddr ||
            offset > length || filesz > length - offset)
            return 0;
        if (align > 1 &&
            ((align & (align - 1)) != 0 || (offset & (align - 1)) != (vaddr & (align - 1))))
            return 0;

        if ((flags & APP_PAYLOAD_PF_X) && filesz &&
            entry >= vaddr && entry - vaddr < filesz)
            entry_loaded = 1;
        load_segments++;
    }

    return load_segments > 0 && entry_loaded;
}

#endif
