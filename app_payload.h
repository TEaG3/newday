#ifndef TEAG3_APP_PAYLOAD_H
#define TEAG3_APP_PAYLOAD_H

#include <stddef.h>

/* A remote catalogue app must be a complete little-endian ELF32 MIPS executable. */
static inline int app_payload_is_valid(const unsigned char *data, size_t length,
                                       unsigned expected_size)
{
    if (data == NULL || expected_size == 0 || length != (size_t)expected_size || length < 52u)
        return 0;
    if (data[0] != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F' ||
        data[4] != 1 || data[5] != 1 || data[6] != 1)
        return 0; /* ELF32, little endian, current version */
    if (data[16] != 2 || data[17] != 0 || data[18] != 8 || data[19] != 0)
        return 0; /* ET_EXEC, EM_MIPS */
    if (data[20] != 1 || data[21] != 0 || data[22] != 0 || data[23] != 0)
        return 0; /* ELF header version */
    return 1;
}

#endif
