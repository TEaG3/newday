#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_payload.h"

#define TEST_ELF_SIZE 100u
#define TEST_PHDR_OFFSET 52u
#define TEST_DATA_OFFSET 84u
#define TEST_MEMORY_LIMIT 0x02000000u

static void put16(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static void put32(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static void make_elf(unsigned char *data, size_t length)
{
    unsigned char *ph;
    memset(data, 0, length);
    data[0] = 0x7f; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 1; data[5] = 1; data[6] = 1;
    put16(data + 16, 2); put16(data + 18, 8); put32(data + 20, 1);
    put32(data + 24, 0x00100004u);
    put32(data + 28, TEST_PHDR_OFFSET);
    put16(data + 40, APP_PAYLOAD_ELF_HEADER_SIZE);
    put16(data + 42, APP_PAYLOAD_ELF_PHDR_SIZE);
    put16(data + 44, 1);

    ph = data + TEST_PHDR_OFFSET;
    put32(ph, APP_PAYLOAD_PT_LOAD);
    put32(ph + 4, TEST_DATA_OFFSET);
    put32(ph + 8, 0x00100000u);
    put32(ph + 12, 0x00100000u);
    put32(ph + 16, 16);
    put32(ph + 20, 32);
    put32(ph + 24, APP_PAYLOAD_PF_X | 4u);
    put32(ph + 28, 16);
}

int main(void)
{
    unsigned char payload[TEST_ELF_SIZE];
    make_elf(payload, sizeof(payload));
    assert(app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    assert(!app_payload_is_valid(payload, sizeof(payload) - 1, sizeof(payload), TEST_MEMORY_LIMIT));
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload) - 1, TEST_MEMORY_LIMIT));
    assert(!app_payload_is_valid(payload, sizeof(payload), 0, TEST_MEMORY_LIMIT));
    assert(!app_payload_is_valid(NULL, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    assert(!app_payload_is_valid(payload, 51, 51, TEST_MEMORY_LIMIT));
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), APP_PAYLOAD_USER_RAM_BASE));

    make_elf(payload, sizeof(payload)); payload[0] = 0;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); payload[4] = 2;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); payload[5] = 2;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); payload[6] = 0;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put16(payload + 16, 3);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put16(payload + 18, 3);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + 20, 2);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put16(payload + 40, 51);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put16(payload + 42, 31);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put16(payload + 44, 0);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + 28, 1000);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + 28, 70);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));

    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET, 0);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET + 16, 33);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET + 4, 99);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET + 8, 0x000fffffu);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET + 8, TEST_MEMORY_LIMIT);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET + 20, TEST_MEMORY_LIMIT);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET + 24, 4);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + 24, 0x00100040u);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));
    make_elf(payload, sizeof(payload)); put32(payload + TEST_PHDR_OFFSET + 28, 3);
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload), TEST_MEMORY_LIMIT));

    puts("remote app ELF32/MIPS payload and PT_LOAD validation: PASS");
    return 0;
}
