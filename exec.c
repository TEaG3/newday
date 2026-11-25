// Run another ELF through loader/loader.elf (embedded): validate the complete ELF32 MIPS image
// before clearing or copying any of the launcher's memory.
#include <string.h>
#include <kernel.h>
#include <sifrpc.h>
#include "exec.h"

extern unsigned char loader_elf[];
extern unsigned int size_loader_elf;

#define LOADER_BASE 0x00084000u
#define LOADER_LIMIT 0x00100000u
#define ELF_HEADER_SIZE 52u
#define ELF32_PHDR_SIZE 32u
#define ELF_PT_LOAD 1u

static u16 elf_u16(const u8 *p)
{
    return (u16)((u16)p[0] | (u16)p[1] << 8);
}

static u32 elf_u32(const u8 *p)
{
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

/* Validate every loadable segment before the first write into EE RAM. */
static int loader_image_valid(const u8 *e, u32 size)
{
    u32 entry, phoff;
    u16 phnum, phentsize;
    int load_segments = 0, entry_loaded = 0;
    if (!e || size < ELF_HEADER_SIZE) return 0;
    if (e[0] != 0x7f || e[1] != 'E' || e[2] != 'L' || e[3] != 'F' ||
        e[4] != 1 || e[5] != 1 || e[6] != 1) return 0; /* ELF32, little endian, current */
    if (elf_u16(e + 16) != 2 || elf_u16(e + 18) != 8 || elf_u32(e + 20) != 1) return 0; /* ET_EXEC/MIPS */
    if (elf_u16(e + 40) != ELF_HEADER_SIZE) return 0;
    phoff = elf_u32(e + 28);
    phentsize = elf_u16(e + 42);
    phnum = elf_u16(e + 44);
    if (phentsize != ELF32_PHDR_SIZE || !phnum ||
        phoff > size || (u32)phnum > (size - phoff) / phentsize) return 0;
    entry = elf_u32(e + 24);
    if (entry < LOADER_BASE || entry >= LOADER_LIMIT) return 0;

    for (u32 k = 0; k < phnum; k++) {
        const u8 *ph = e + phoff + k * phentsize;
        u32 type = elf_u32(ph);
        u32 offset, vaddr, filesz, memsz;
        if (type != ELF_PT_LOAD) continue;
        offset = elf_u32(ph + 4);
        vaddr = elf_u32(ph + 8);
        filesz = elf_u32(ph + 16);
        memsz = elf_u32(ph + 20);
        if (!memsz || filesz > memsz || vaddr < LOADER_BASE ||
            vaddr >= LOADER_LIMIT || memsz > LOADER_LIMIT - vaddr ||
            offset > size || filesz > size - offset) return 0;
        if (entry >= vaddr && entry - vaddr < memsz) entry_loaded = 1;
        load_segments++;
    }
    return load_segments > 0 && entry_loaded;
}

void run_loader(int argc, char *argv[])
{
    const u8 *e = loader_elf;
    if (argc < 2 || !argv) return;
    for (int k = 0; k < argc; k++)
        if (!argv[k]) return;
    if (!loader_image_valid(e, size_loader_elf)) return;

    /* Only after all ELF offsets, sizes and addresses have passed validation. */
    memset((void *)LOADER_BASE, 0, LOADER_LIMIT - LOADER_BASE);
    u32 phoff = elf_u32(e + 28);
    u16 phnum = elf_u16(e + 44);
    u16 phentsize = elf_u16(e + 42);
    for (u32 k = 0; k < phnum; k++) {
        const u8 *ph = e + phoff + k * phentsize;
        if (elf_u32(ph) == ELF_PT_LOAD) {
            u32 offset = elf_u32(ph + 4);
            u32 vaddr = elf_u32(ph + 8);
            u32 filesz = elf_u32(ph + 16);
            if (filesz) memcpy((void *)vaddr, e + offset, filesz);
        }
    }
    SifExitRpc();
    FlushCache(0);
    FlushCache(2);
    ExecPS2((void *)elf_u32(e + 24), NULL, argc, argv);
}
