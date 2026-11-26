// OPL-only source discovery. No secondary game-runtime modules are loaded.
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <kernel.h>
#include <loadfile.h>
#include <hdd-ioctl.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include "sources.h"
#include "net.h"
#include "lang.h"

#define IRX(m) extern unsigned char m##_irx[]; extern unsigned int size_##m##_irx
IRX(ata_bd); IRX(ps2hdd_bdm); IRX(mx4sio_bd_mini); IRX(iLinkman); IRX(IEEE1394_bd_mini); IRX(mmceman);

const char *const src_key[SRC_N] = {"usb", "hdd", "hdl", "mx4sio", "ilink", "mmce", "udpbd", "udpfs", "smb"};
const char *const src_label[SRC_N] = {"USB", "HDD", "HDD", "MX4SIO", "ILINK", "MMCE", "UDPBD", "UDPFS", "SMB"};
source src[SRC_MAX];
int nsrc;
const char *src_err[SRC_N];
#define FAIL(t, msg) (why = src_err[t] = (msg))

unsigned src_mask(const char *list)
{
    unsigned m = 0;
    char w[16];
    while (*list) {
        int n = 0;
        while (*list == ',' || *list == ' ' || *list == '\t') list++;
        while (*list && *list != ',' && *list != ' ' && *list != '\t' && n < 15) w[n++] = *list++;
        w[n] = 0;
        for (int s = 0; s < SRC_N; s++)
            if (n && !strcasecmp(w, src_key[s])) m |= 1u << s;
        while (*list && *list != ',' && *list != ' ' && *list != '\t') list++;
    }
    if (m & (1u << SRC_HDL)) m |= 1u << SRC_HDD;
    return m;
}
static int load(void *irx, unsigned size, const char *args, int argl)
{
    int r = 0;
    return SifExecModuleBuffer(irx, size, argl, args, &r) >= 0 && r != 1;
}
static int mounted(const char *root, int quarters)
{
    char dir[16];
    snprintf(dir, sizeof(dir), strncmp(root, "hdd", 3) ? "%s/" : "%s", root);
    for (int t = 0;; t++) {
        DIR *d = opendir(dir);
        if (d) { closedir(d); return 1; }
        if (t >= quarters) return 0;
        usleep(250000);
    }
}
static int add(int type, const char *root, int quarters)
{
    if (nsrc >= SRC_MAX || !mounted(root, quarters)) return 0;
    src[nsrc].type = type;
    src[nsrc].unit = isdigit((u8)root[strlen(root) - 2]) ? root[strlen(root) - 2] - '0' : 0;
    snprintf(src[nsrc].root, sizeof(src[nsrc].root), "%s", root);
    printf("sources: %s at %s\n", src_key[type], root);
    nsrc++;
    return 1;
}
static int mmce_up;
static int mmce_load(void)
{
    if (!mmce_up) mmce_up = load(mmceman_irx, size_mmceman_irx, NULL, 0) ? 1 : -1;
    return mmce_up > 0;
}
int src_home(const char *root)
{
    return !strncmp(root, "mmce", 4) && mmce_load() && mounted(root, 20);
}
static int apa_disk(void)
{
    static u8 sec[512] __attribute__((aligned(64)));
    hddAtaTransfer_t a = {0, 1};
    return fileXioDevctl("hdd0:", HDIOC_READSECTOR, &a, sizeof(a), sec, 512) >= 0 && !memcmp(sec + 4, "APA", 3);
}

const char *src_init(unsigned mask, const char *ip, int udpfs_mount)
{
    (void)ip;
    const char *why = NULL;
    if (mask & (1u << SRC_HDD)) {
        if (!net_dev9() || !load(ata_bd_irx, size_ata_bd_irx, NULL, 0))
            FAIL(SRC_HDD, L("Could not load the hard disk driver"));
        else if (!add(SRC_HDD, "ata0:", 12)) {
            static const char a[] = "-o\0" "4\0" "-n\0" "20";
            if (load(ps2hdd_bdm_irx, size_ps2hdd_bdm_irx, a, sizeof(a)) &&
                mounted("hdd0:", 8) && apa_disk()) {
                src[nsrc].type = SRC_HDL;
                src[nsrc].unit = 0;
                strcpy(src[nsrc].root, "hdd0:");
                nsrc++;
            } else FAIL(SRC_HDD, L("Hard disk not found (FAT32 or HD Loader)"));
        }
    }
    if ((mask & (1u << SRC_USB)) && !add(SRC_USB, "mass0:", 20))
        FAIL(SRC_USB, L("USB drive not found"));
    if ((mask & (1u << SRC_MX4SIO)) &&
        (!load(mx4sio_bd_mini_irx, size_mx4sio_bd_mini_irx, NULL, 0) || !add(SRC_MX4SIO, "mx4sio0:", 12)))
        FAIL(SRC_MX4SIO, L("MX4SIO card not found"));
    if ((mask & (1u << SRC_ILINK)) &&
        (!load(iLinkman_irx, size_iLinkman_irx, NULL, 0) ||
         !load(IEEE1394_bd_mini_irx, size_IEEE1394_bd_mini_irx, NULL, 0) || !add(SRC_ILINK, "ilink0:", 16)))
        FAIL(SRC_ILINK, L("iLink disk not found"));

    /* UDPFS is retained only as an HTTP catalogue for downloadable APPS, never as a game filesystem. */
    if ((mask & (1u << SRC_UDPFS)) && !udpfs_mount && nsrc < SRC_MAX) {
        src[nsrc].type = SRC_UDPFS;
        src[nsrc].unit = 0;
        strcpy(src[nsrc].root, "udpfs:");
        nsrc++;
    } else if (mask & (1u << SRC_UDPFS)) {
        src_err[SRC_UDPFS] = L("UDPFS game streaming is not supported by the OPL-only launcher");
    }
    if (mask & (1u << SRC_MMCE)) src_err[SRC_MMCE] = L("MMCE game launching is not yet adapted to OPL");
    if (mask & (1u << SRC_UDPBD)) src_err[SRC_UDPBD] = L("UDPBD game launching is not supported by OPL integration");
    if (mask & (1u << SRC_SMB)) src_err[SRC_SMB] = L("SMB discovery is disabled until an OPL-only reader is integrated");
    return why;
}

// HD Loader metadata, matching upstream OPL's hddGetHDLGamelist() calculation.
typedef struct hdl_scan_record {
	struct hdl_scan_record *next;
	char part[APA_IDMAX + 1], title[161], startup[61];
	u32 start_sector, total_size_kb, disc_type;
	int is_cd;
} hdl_scan_record;

int src_hdl_scan(void (*cb)(void *ctx, const char *part, const char *title, const char *startup, u32 start_sector, u32 total_size_kb, u32 disc_type, int is_cd), void *ctx)
{
	static u8 h[1024] __attribute__((aligned(64)));
	iox_dirent_t de;
	hdl_scan_record *head = NULL, **tail = &head, *g;
	int fd = fileXioDopen("hdd0:"), n = 0, failed = 0;
	if (fd < 0) return 0;
	while (fileXioDread(fd, &de) > 0) {
		if (de.stat.mode != 0x1337 || (de.stat.attr & APA_FLAG_SUB)) continue;
		if (de.stat.private_5 > 0xffffffffu - 2064u) continue;
		hddAtaTransfer_t a = {de.stat.private_5 + 2064u, 2};
		if (fileXioDevctl("hdd0:", HDIOC_READSECTOR, &a, sizeof(a), h, 1024) != 0 ||
		    *(u32 *)h != 0xDEADFEED) continue;
		h[8 + 159] = 0, h[172 + 59] = 0;
		if (strlen((const char *)h + 172) != 11 || de.stat.size / 2u == 0) continue;
		g = calloc(1, sizeof(*g));
		if (!g) { failed = 1; break; }
		snprintf(g->part, sizeof(g->part), "%.*s", APA_IDMAX, de.name);
		snprintf(g->title, sizeof(g->title), "%.*s", 159, (const char *)h + 8);
		snprintf(g->startup, sizeof(g->startup), "%s", (const char *)h + 172);
		g->start_sector = a.lba;
		g->disc_type = *(u32 *)(h + 236);
		g->is_cd = ((u8)g->disc_type == 0x12 || (u8)g->disc_type == 0x13); // SCECdPS2CD / SCECdPS2CDDA
		g->total_size_kb = de.stat.size / 2u; // OPL: (APA sectors / 4) * 2
		*tail = g;
		tail = &g->next;
	}
	fileXioDclose(fd);
	if (failed) {
		while (head) { g = head->next; free(head); head = g; }
		return 0;
	}

	/* OPL includes every APA sub-partition in total_size_in_kb; aggregate them by APA_IDMAX. */
	fd = fileXioDopen("hdd0:");
	if (fd < 0) {
		while (head) { g = head->next; free(head); head = g; }
		return 0;
	}
	{
		while (fileXioDread(fd, &de) > 0) {
			if (de.stat.mode != 0x1337 || !(de.stat.attr & APA_FLAG_SUB)) continue;
			for (g = head; g; g = g->next) {
				if (!strncmp(g->part, de.name, APA_IDMAX)) {
					u32 add_kb = de.stat.size / 2u;
					if (g->total_size_kb && g->total_size_kb <= 0xffffffffu - add_kb)
						g->total_size_kb += add_kb;
					else
						g->total_size_kb = 0;
					break;
				}
			}
		}
		fileXioDclose(fd);
	}
	for (g = head; g; ) {
		hdl_scan_record *next = g->next;
		if (g->total_size_kb && cb) {
			cb(ctx, g->part, g->title, g->startup, g->start_sector, g->total_size_kb, g->disc_type, g->is_cd);
			n++;
		}
		free(g);
		g = next;
	}
	return n;
}
