#pragma once
#include <tamtypes.h>
/* OPL-only game source discovery. Legacy IDs remain for safe parsing of older config files. */
enum { SRC_USB, SRC_HDD, SRC_HDL, SRC_MX4SIO, SRC_ILINK, SRC_MMCE, SRC_UDPBD, SRC_UDPFS, SRC_SMB, SRC_N };
#define SRC_MAX SRC_N
typedef struct { char type, unit; char root[10]; } source;
extern source src[SRC_MAX];
extern int nsrc;
extern const char *src_err[SRC_N];
extern const char *const src_key[SRC_N];
extern const char *const src_label[SRC_N];
int src_home(const char *root);
unsigned src_mask(const char *list);
const char *src_init(unsigned mask, const char *ip, int udpfs_mount);
enum {
    SRC_HDL_SCAN_ERR_OPEN = -1,
    SRC_HDL_SCAN_ERR_READ = -2,
    SRC_HDL_SCAN_ERR_CLOSE = -3,
    SRC_HDL_SCAN_ERR_MEMORY = -4,
    SRC_HDL_SCAN_ERR_METADATA = -5
};
/* Returns a game count only after both APA directory passes and all closes succeed.
   Negative SRC_HDL_SCAN_ERR_* values mean the source is incomplete; no callbacks are made. */
int src_hdl_scan(void (*add)(void *ctx, const char *part, const char *title, const char *startup,
                             u32 start_sector, u32 total_size_kb, u32 disc_type, int is_cd), void *ctx);
