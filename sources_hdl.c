/* Isolated APA/HD Loader catalogue scanner so host tests can exercise I/O failures. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <hdd-ioctl.h>
#include "sources.h"

typedef struct hdl_scan_record {
	struct hdl_scan_record *next;
	char part[APA_IDMAX + 1], title[161], startup[61];
	u32 start_sector, total_size_kb, disc_type;
	int is_cd;
} hdl_scan_record;

static void hdl_scan_free(hdl_scan_record *head)
{
    while (head) {
        hdl_scan_record *next = head->next;
        free(head);
        head = next;
    }
}

static int hdl_scan_cancelled(src_hdl_cancel_fn cancelled, void *context)
{
    return cancelled != NULL && cancelled(context);
}

/* HD Loader metadata, matching upstream OPL's hddGetHDLGamelist() calculation.
   A partial directory/sector read is not a valid catalogue: do not call the consumer
   until both passes and their directory handles have completed successfully. */
int src_hdl_scan_cancelable(void (*cb)(void *ctx, const char *part, const char *title, const char *startup, u32 start_sector, u32 total_size_kb, u32 disc_type, int is_cd), void *ctx, src_hdl_cancel_fn cancelled, void *cancel_context)
{
    static u8 h[1024] __attribute__((aligned(64)));
    iox_dirent_t de;
    hdl_scan_record *head = NULL, **tail = &head, *g;
    int fd, n = 0, status = 0, read_result = 0;

    if (hdl_scan_cancelled(cancelled, cancel_context))
        return SRC_HDL_SCAN_ERR_CANCELLED;
    fd = fileXioDopen("hdd0:");
    if (fd < 0) return SRC_HDL_SCAN_ERR_OPEN;
    for (;;) {
        if (hdl_scan_cancelled(cancelled, cancel_context)) {
            status = SRC_HDL_SCAN_ERR_CANCELLED;
            break;
        }
        read_result = fileXioDread(fd, &de);
        if (read_result < 0) {
            status = SRC_HDL_SCAN_ERR_READ;
            break;
        }
        if (read_result == 0) break;
        if (hdl_scan_cancelled(cancelled, cancel_context)) {
            status = SRC_HDL_SCAN_ERR_CANCELLED;
            break;
        }
        if (de.stat.mode != 0x1337 || (de.stat.attr & APA_FLAG_SUB)) continue;
        if (de.stat.private_5 > 0xffffffffu - 2064u) {
            status = SRC_HDL_SCAN_ERR_METADATA;
            break;
        }
        u64 base_size_kb = de.stat.size / 2u;
        if (base_size_kb > 0xffffffffu) {
            status = SRC_HDL_SCAN_ERR_METADATA;
            break;
        }
        hddAtaTransfer_t a = {de.stat.private_5 + 2064u, 2};
        if (fileXioDevctl("hdd0:", HDIOC_READSECTOR, &a, sizeof(a), h, 1024) != 0) {
            status = SRC_HDL_SCAN_ERR_READ;
            break;
        }
        if (hdl_scan_cancelled(cancelled, cancel_context)) {
            status = SRC_HDL_SCAN_ERR_CANCELLED;
            break;
        }
        if (*(u32 *)h != 0xDEADFEED) continue;
        h[8 + 159] = 0, h[172 + 59] = 0;
        if (strlen((const char *)h + 172) != 11 || de.stat.size / 2u == 0) continue;
        g = calloc(1, sizeof(*g));
        if (!g) {
            status = SRC_HDL_SCAN_ERR_MEMORY;
            break;
        }
        snprintf(g->part, sizeof(g->part), "%.*s", APA_IDMAX, de.name);
        snprintf(g->title, sizeof(g->title), "%.*s", 159, (const char *)h + 8);
        snprintf(g->startup, sizeof(g->startup), "%s", (const char *)h + 172);
        g->start_sector = a.lba;
        g->disc_type = *(u32 *)(h + 236);
        g->is_cd = ((u8)g->disc_type == 0x12 || (u8)g->disc_type == 0x13); // SCECdPS2CD / SCECdPS2CDDA
        g->total_size_kb = (u32)base_size_kb; // OPL: (APA sectors / 4) * 2
        *tail = g;
        tail = &g->next;
    }
    if (read_result < 0 && status == 0) status = SRC_HDL_SCAN_ERR_READ;
    if (fileXioDclose(fd) < 0 && status == 0) status = SRC_HDL_SCAN_ERR_CLOSE;
    if (status != 0) {
        hdl_scan_free(head);
        return status;
    }

    /* OPL includes every APA sub-partition in total_size_in_kb; aggregate them by APA_IDMAX. */
    if (hdl_scan_cancelled(cancelled, cancel_context)) {
        hdl_scan_free(head);
        return SRC_HDL_SCAN_ERR_CANCELLED;
    }
    fd = fileXioDopen("hdd0:");
    if (fd < 0) {
        hdl_scan_free(head);
        return SRC_HDL_SCAN_ERR_OPEN;
    }
    for (;;) {
        if (hdl_scan_cancelled(cancelled, cancel_context)) {
            status = SRC_HDL_SCAN_ERR_CANCELLED;
            break;
        }
        read_result = fileXioDread(fd, &de);
        if (read_result < 0) {
            status = SRC_HDL_SCAN_ERR_READ;
            break;
        }
        if (read_result == 0) break;
        if (hdl_scan_cancelled(cancelled, cancel_context)) {
            status = SRC_HDL_SCAN_ERR_CANCELLED;
            break;
        }
        if (de.stat.mode != 0x1337 || !(de.stat.attr & APA_FLAG_SUB)) continue;
        for (g = head; g; g = g->next) {
            if (!strncmp(g->part, de.name, APA_IDMAX)) {
                u64 add_kb64 = de.stat.size / 2u;
                if (add_kb64 > 0xffffffffu || !g->total_size_kb ||
                    g->total_size_kb > 0xffffffffu - (u32)add_kb64) {
                    status = SRC_HDL_SCAN_ERR_METADATA;
                } else {
                    g->total_size_kb += (u32)add_kb64;
                }
                break;
            }
        }
        if (status != 0) break;
    }
    if (read_result < 0 && status == 0) status = SRC_HDL_SCAN_ERR_READ;
    if (fileXioDclose(fd) < 0 && status == 0) status = SRC_HDL_SCAN_ERR_CLOSE;
    if (status != 0) {
        hdl_scan_free(head);
        return status;
    }

    if (hdl_scan_cancelled(cancelled, cancel_context)) {
        hdl_scan_free(head);
        return SRC_HDL_SCAN_ERR_CANCELLED;
    }
    for (g = head; g; g = g->next) {
        if (!g->total_size_kb) {
            hdl_scan_free(head);
            return SRC_HDL_SCAN_ERR_METADATA;
        }
    }
    for (g = head; g; ) {
        hdl_scan_record *next = g->next;
        if (cb) cb(ctx, g->part, g->title, g->startup, g->start_sector,
                    g->total_size_kb, g->disc_type, g->is_cd);
        n++;
        free(g);
        g = next;
    }
    return n;
}

int src_hdl_scan(void (*cb)(void *ctx, const char *part, const char *title, const char *startup, u32 start_sector, u32 total_size_kb, u32 disc_type, int is_cd), void *ctx)
{
    return src_hdl_scan_cancelable(cb, ctx, NULL, NULL);
}
