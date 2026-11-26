#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_payload.h"

static void make_elf(unsigned char *data, size_t length)
{
    memset(data, 0, length);
    data[0] = 0x7f; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 1; data[5] = 1; data[6] = 1;
    data[16] = 2; data[18] = 8; data[20] = 1;
}

int main(void)
{
    unsigned char payload[64];
    make_elf(payload, sizeof(payload));
    assert(app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));
    assert(!app_payload_is_valid(payload, sizeof(payload) - 1, sizeof(payload)));
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload) - 1));
    assert(!app_payload_is_valid(payload, sizeof(payload), 0));
    assert(!app_payload_is_valid(NULL, sizeof(payload), sizeof(payload)));
    assert(!app_payload_is_valid(payload, 51, 51));

    payload[0] = 0;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));
    make_elf(payload, sizeof(payload)); payload[4] = 2;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));
    make_elf(payload, sizeof(payload)); payload[5] = 2;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));
    make_elf(payload, sizeof(payload)); payload[6] = 0;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));
    make_elf(payload, sizeof(payload)); payload[16] = 3;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));
    make_elf(payload, sizeof(payload)); payload[18] = 3;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));
    make_elf(payload, sizeof(payload)); payload[20] = 2;
    assert(!app_payload_is_valid(payload, sizeof(payload), sizeof(payload)));

    puts("remote app payload validation: PASS");
    return 0;
}
