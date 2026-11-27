#ifndef TEAG3_UL_H
#define TEAG3_UL_H

#define UL_CFG_RECORD_SIZE 64
#define UL_NAME_SIZE 32
#define UL_STARTUP_SIZE 12
#define UL_PART_LIMIT 255

typedef struct {
    char name[UL_NAME_SIZE + 1];      /* Display title, trailing spaces removed. */
    char raw_name[UL_NAME_SIZE + 1];  /* Exact OPL CRC input, including padding. */
    char startup[UL_STARTUP_SIZE + 1];
    unsigned char parts;
    unsigned char media;
    unsigned int name_crc;
} ul_game;

typedef int (*ul_part_exists_fn)(void *context, const char *filename);

/* Returns 1 only when the record is structurally valid and every part exists. */
int ul_decode_record(const unsigned char record[UL_CFG_RECORD_SIZE],
                     ul_game *game, ul_part_exists_fn part_exists, void *context);
unsigned int ul_name_crc32(const char *name);
/* Returns false without overflowing when incoming records do not fit. */
int ul_catalog_capacity_fits(unsigned int incoming, unsigned int existing, unsigned int capacity);

#endif
