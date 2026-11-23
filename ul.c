#include "ul.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static unsigned int crc_table[256];
static int crc_ready;

static void ul_crc_init(void)
{
    int table, count;
    unsigned int crc;
    if (crc_ready) return;
    for (table = 0; table < 256; table++) {
        crc = (unsigned int)table << 24;
        for (count = 8; count > 0; count--) {
            if (crc & 0x80000000u) crc <<= 1;
            else crc = (crc << 1) ^ 0x04C11DB7u;
        }
        crc_table[255 - table] = crc;
    }
    crc_ready = 1;
}

/* Exact OPL USBA_crc32 algorithm (non-reflected, 0x04C11DB7, zero initial state). */
unsigned int ul_name_crc32(const char *name)
{
    unsigned int crc;
    int count = 0;
    ul_crc_init();
    /* OPL's table construction leaves the initial accumulator at table[0]. */
    crc = crc_table[0];
    do {
        unsigned int byte = (unsigned char)name[count++];
        crc = crc_table[byte ^ ((crc >> 24) & 0xFFu)] ^ (crc << 8);
    } while (name[count - 1] != '\0' && count <= UL_NAME_SIZE);
    return crc;
}

static int valid_startup(const unsigned char *p)
{
    int i;
    /* PS2 serial in the form SLUS_209.46 (11 bytes). */
    for (i = 0; i < 11; i++) {
        unsigned char c = p[i];
        if (i == 4) {
            if (c != '_') return 0;
        } else if (i == 8) {
            if (c != '.') return 0;
        } else if (!isalnum(c)) return 0;
    }
    return p[11] == 0;
}

int ul_decode_record(const unsigned char record[UL_CFG_RECORD_SIZE],
                     ul_game *game, ul_part_exists_fn part_exists, void *context)
{
    char partname[40];
    unsigned int crc;
    unsigned int part;
    size_t title_len = UL_NAME_SIZE;

    if (!record || !game || !part_exists) return 0;
    if (memcmp(record + 32, "ul.", 3) != 0) return 0;
    if (record[53] != 0x08) return 0;
    if (record[47] == 0 || !valid_startup(record + 35)) return 0;
    if (record[48] != 0x12 && record[48] != 0x14) return 0;

    memset(game, 0, sizeof(*game));
    memcpy(game->raw_name, record, UL_NAME_SIZE);
    game->raw_name[UL_NAME_SIZE] = 0;
    memcpy(game->name, record, UL_NAME_SIZE);
    game->name[UL_NAME_SIZE] = 0;
    while (title_len && (game->name[title_len - 1] == ' ' || game->name[title_len - 1] == 0))
        game->name[--title_len] = 0;
    if (!title_len) return 0;

    memcpy(game->startup, record + 35, UL_STARTUP_SIZE);
    game->startup[UL_STARTUP_SIZE] = 0;
    game->parts = record[47];
    game->media = record[48];
    crc = ul_name_crc32(game->raw_name);
    game->name_crc = crc;

    for (part = 0; part < game->parts; part++) {
        snprintf(partname, sizeof(partname), "ul.%08X.%s.%02x", crc, game->startup, part);
        if (!part_exists(context, partname)) return 0;
    }
    return 1;
}

#if defined(SELFTEST)
#include <assert.h>
#include <stdlib.h>

typedef struct {
    char names[4][40];
    int count;
} fake_parts;

static int fake_exists(void *context, const char *name)
{
    fake_parts *parts = (fake_parts *)context;
    int i;
    for (i = 0; i < parts->count; i++)
        if (strcmp(parts->names[i], name) == 0) return 1;
    return 0;
}

static void make_record(unsigned char record[UL_CFG_RECORD_SIZE], const char *title,
                        const char *startup, unsigned char media, unsigned char part_count)
{
    memset(record, 0, UL_CFG_RECORD_SIZE);
    memcpy(record, title, strlen(title) < UL_NAME_SIZE ? strlen(title) : UL_NAME_SIZE);
    memcpy(record + 32, "ul.", 3);
    memcpy(record + 35, startup, strlen(startup) < UL_STARTUP_SIZE ? strlen(startup) : UL_STARTUP_SIZE);
    record[47] = part_count;
    record[48] = media;
    record[53] = 0x08;
}

int main(void)
{
    unsigned char record[UL_CFG_RECORD_SIZE];
    ul_game game;
    fake_parts parts = {{{0}}, 0};
    char title[33] = "Test Game";
    unsigned int crc;

    make_record(record, title, "SLUS_209.46", 0x14, 2);
    memset(&game, 0, sizeof(game));
    crc = ul_name_crc32("Test Game");
    snprintf(parts.names[0], sizeof(parts.names[0]), "ul.%08X.SLUS_209.46.00", crc);
    snprintf(parts.names[1], sizeof(parts.names[1]), "ul.%08X.SLUS_209.46.01", crc);
    parts.count = 2;

    assert(ul_decode_record(record, &game, fake_exists, &parts));
    assert(strcmp(game.name, "Test Game") == 0);
    assert(strcmp(game.startup, "SLUS_209.46") == 0);
    assert(game.parts == 2 && game.media == 0x14);
    assert(game.name_crc == crc);

    parts.count = 1;
    assert(!ul_decode_record(record, &game, fake_exists, &parts)); /* missing second split part */
    parts.count = 2;

    record[32] = 'x';
    assert(!ul_decode_record(record, &game, fake_exists, &parts)); /* bad magic */
    record[32] = 'u';
    record[53] = 0;
    assert(!ul_decode_record(record, &game, fake_exists, &parts)); /* bad marker */
    record[53] = 0x08;
    record[47] = 0;
    assert(!ul_decode_record(record, &game, fake_exists, &parts)); /* zero parts */
    record[47] = 2;
    record[48] = 0x99;
    assert(!ul_decode_record(record, &game, fake_exists, &parts)); /* unknown media */
    record[48] = 0x14;
    record[46] = 'X';
    assert(!ul_decode_record(record, &game, fake_exists, &parts)); /* invalid startup terminator */
    puts("ul selftest ok");
    return 0;
}
#endif
