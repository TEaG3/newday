// Game sources (phase 14). Every block device goes through the BDM the launcher already runs for its USB
// (bdm + bdmfs_fatfs, iop.c), which mounts each one under its driver's name as well as massN: (ps2sdk bdmfs_fatfs
// fs_driver.c: bd->path "usb", "ata", "mx4sio", "ilink"; Neutrino's udpbd "udpbd"). Those names are the paths
// Neutrino takes (-dvd=ata:DVD/x.iso, README "Usage examples") and maps to its -bsd. Drivers per source as Neutrino
// v1.8.0's config/bsd-*.toml and nhddl src/devices/init.c (reference only, docs/sources.md):
//   hdd     ps2dev9 + ata_bd (exFAT/FAT disk), else ps2hdd-bdm (-o 4 -n 20) for an APA disk with HD Loader games
//   mx4sio  mx4sio_bd_mini        ilink  iLinkman + IEEE1394_bd_mini        mmce  mmceman (mmce0: / mmce1:)
//   udpbd   ps2dev9 + smap + ministack ip= + udpbd          udpfs  file-system catalog or udpfs_ioman (phase 17);
//   smb     ps2dev9 + smap + ministack + smb_ioman; read-only SMB share with a TEAG3.GAMES index (Neutrino SMB FHI)
// The ps2sdk ones are embedded (same build as our bdm); smap / ministack / udpbd / udpfs_ioman exist only in
// Neutrino, so they are read from its modules/ folder. With -qb these stay loaded under Neutrino's load stage.
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
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
const char *const src_bsd[SRC_N] = {"usb", "ata", "ata", "mx4sio", "ilink", "mmce", "udpbd", "udpfs", "smb"};
const char *const src_label[SRC_N] = {"USB", "HDD", "HDD", "MX4SIO", "ILINK", "MMCE", "UDPBD", "UDPFS", "SMB"};
source src[SRC_MAX];
int nsrc;
const char *src_err[SRC_N];
#define FAIL(t, msg) (why = src_err[t] = (msg)) // every wanted source keeps its own reason (#4); why: the first

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
		while (*list && *list != ',' && *list != ' ' && *list != '\t') list++; // a word longer than 15
	}
	if (m & 1u << SRC_HDL) m |= 1u << SRC_HDD; // "hdl" is the same disk driver
	return m;
}

static int load(void *irx, unsigned size, const char *args, int argl)
{
	int r = 0;
	return SifExecModuleBuffer(irx, size, argl, args, &r) >= 0 && r != 1; // 1 = NO_RESIDENT_END: did not stay
}

static int load_file(const char *ndir, const char *name, const char *args, int argl) // one of Neutrino's modules
{
	char path[96];
	snprintf(path, sizeof(path), "%s/modules/%s", ndir, name);
	FILE *f = fopen(path, "rb");
	if (!f) { printf("sources: %s missing\n", path); return 0; }
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	void *b = n > 0 && n < (1 << 20) ? memalign(64, n) : NULL;
	int ok = b && fread(b, n, 1, f) == 1;
	fclose(f);
	if (ok) ok = load(b, n, args, argl);
	free(b);
	printf("sources: %s %s\n", path, ok ? "loaded" : "failed");
	return ok;
}

static int mounted(const char *root, int quarters) // waits up to quarters x 250 ms for root to open
{
	char dir[16];
	snprintf(dir, sizeof(dir), strncmp(root, "hdd", 3) ? "%s/" : "%s", root); // ps2hdd lists partitions at "hdd0:"
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
	src[nsrc].type = type, src[nsrc].unit = isdigit((u8)root[strlen(root) - 2]) ? root[strlen(root) - 2] - '0' : 0; // "udpfs:": 0
	snprintf(src[nsrc].root, sizeof(src[nsrc].root), "%s", root);
	printf("sources: %s at %s\n", src_key[type], root);
	nsrc++;
	return 1;
}

static int mmce_up; // mmceman: 0 not tried, 1 loaded, -1 failed (loaded once: the home may have needed it first)
static int mmce_load(void)
{
	if (!mmce_up) mmce_up = load(mmceman_irx, size_mmceman_irx, NULL, 0) ? 1 : -1;
	return mmce_up > 0;
}

int src_home(const char *root) // #8: NULD started from an MMCE: its driver before config.ini; 0 for the USB
{
	return !strncmp(root, "mmce", 4) && mmce_load() && mounted(root, 20);
}

static int apa_disk(void) // sector 0 of hdd0: carries "APA" at byte 4 (nhddl hdl.c checkAPAHeader)
{
	static u8 sec[512] __attribute__((aligned(64)));
	hddAtaTransfer_t a = {0, 1};
	return fileXioDevctl("hdd0:", HDIOC_READSECTOR, &a, sizeof(a), sec, 512) >= 0 && !memcmp(sec + 4, "APA", 3);
}

// Neutrino's ministack takes its IP from the toml for the game stage: keep it equal to ours (-qb loads ours only
// for the launcher's own stage). Rewrites "ip=..." in place when it differs.
static void neutrino_ip(const char *ndir, const char *name, const char *ip)
{
	char path[96], buf[4096], out[4200];
	snprintf(path, sizeof(path), "%s/config/%s", ndir, name);
	FILE *f = fopen(path, "rb");
	if (!f) return;
	int n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	if (n <= 0 || n == sizeof(buf) - 1) return; // ponytail: Neutrino's tomls are under 1 KB; never write a cut one
	buf[n] = 0;
	char *p = strstr(buf, "\"ip="), *e = p ? strchr(p + 4, '"') : NULL;
	if (!e || ((int)strlen(ip) == e - p - 4 && !strncmp(p + 4, ip, e - p - 4))) return;
	snprintf(out, sizeof(out), "%.*s\"ip=%s%s", (int)(p - buf), buf, ip, e);
	if ((f = fopen(path, "wb"))) fputs(out, f), fclose(f), printf("sources: %s now ip=%s\n", path, ip);
}

const char *src_init(unsigned mask, const char *ip, const char *ndir, int udpfs_mount)
{
	const char *why = NULL;
	if (mask & 1u << SRC_HDD && (!net_dev9() || !load(ata_bd_irx, size_ata_bd_irx, NULL, 0)))
		FAIL(SRC_HDD, L("No se pudo cargar el driver del disco duro", "Could not load the hard disk driver"));
	else if (mask & 1u << SRC_HDD) {
		if (!add(SRC_HDD, "ata0:", 12)) { // no FAT / exFAT partition: an APA disk with HD Loader games?
			static const char a[] = "-o\0" "4\0" "-n\0" "20"; // 4 descriptors, 20 buffers (Neutrino bsdfs-hdl.toml)
			if (load(ps2hdd_bdm_irx, size_ps2hdd_bdm_irx, a, sizeof(a)) && mounted("hdd0:", 8) && apa_disk())
				src[nsrc].type = SRC_HDL, src[nsrc].unit = 0, strcpy(src[nsrc].root, "hdd0:"), nsrc++;
			else FAIL(SRC_HDD, L("No se encontró el disco duro (exFAT o HD Loader)", "Hard disk not found (exFAT or HD Loader)"));
		}
	}
	if (mask & 1u << SRC_USB && !add(SRC_USB, "mass0:", 20)) // only when NULD runs from an MMCE (#8)
		FAIL(SRC_USB, L("No se encontró la USB", "USB drive not found"));
	if (mask & 1u << SRC_MX4SIO && mmce_up > 0) // NULD runs from an MMCE: same SIO2 port
		mask &= ~(1u << SRC_MX4SIO), FAIL(SRC_MX4SIO, L("MX4SIO no va con TEa.G3 en un MMCE", "MX4SIO does not go with TEa.G3 on an MMCE"));
	if (mask & 1u << SRC_MX4SIO && mask & 1u << SRC_MMCE) // both on the SIO2 of slot 2 (nhddl README)
		mask &= ~(1u << SRC_MMCE), FAIL(SRC_MMCE, L("MX4SIO y MMCE no van juntos: se usa MX4SIO", "MX4SIO and MMCE do not go together: using MX4SIO"));
	if (mask & 1u << SRC_MX4SIO && (!load(mx4sio_bd_mini_irx, size_mx4sio_bd_mini_irx, NULL, 0) || !add(SRC_MX4SIO, "mx4sio0:", 12)))
		FAIL(SRC_MX4SIO, L("No se encontró la tarjeta del MX4SIO", "MX4SIO card not found"));
	if (mask & 1u << SRC_ILINK && (!load(iLinkman_irx, size_iLinkman_irx, NULL, 0) ||
	                               !load(IEEE1394_bd_mini_irx, size_IEEE1394_bd_mini_irx, NULL, 0) || !add(SRC_ILINK, "ilink0:", 16)))
		FAIL(SRC_ILINK, L("No se encontró el disco iLink", "iLink disk not found"));
	// mmceman also when the MMCE is no game source: a MemCard PRO 2 / SD2PSX gets the game ID (#2, as nhddl)
	int mmce = !(mask & 1u << SRC_MX4SIO) && mmce_load();
	if (mask & 1u << SRC_MMCE && mmce) {
		int any = add(SRC_MMCE, "mmce0:", 4);
		if (!(any |= add(SRC_MMCE, "mmce1:", 0))) FAIL(SRC_MMCE, L("No se encontró ningún MMCE", "No MMCE found"));
	} else if (mask & 1u << SRC_MMCE) FAIL(SRC_MMCE, L("No se pudo cargar el driver del MMCE", "Could not load the MMCE driver"));
	if (mask & 1u << SRC_UDPFS && !udpfs_mount && nsrc < SRC_MAX) // phase 17: listed from the server's catalog
		src[nsrc].type = SRC_UDPFS, src[nsrc].unit = 0, strcpy(src[nsrc].root, "udpfs:"), nsrc++;
	// Neutrino's smap + ministack, shared by udpbd and a mounted udpfs (any udpfs server, as nhddl: #10)
	unsigned udp = mask & (1u << SRC_UDPBD | (udpfs_mount ? 1u << SRC_UDPFS : 0));
	const char *fail = NULL;
	char arg[24];
	int n = snprintf(arg, sizeof(arg), "ip=%s", ip) + 1;
	if (udp && (!strcasecmp(ip, "dhcp") || !*ip)) // ministack has no DHCP (catalog udpfs takes the lease, src_udpfs_ip)
		fail = L("UDPBD / UDPFS sin catálogo necesitan una IP fija en [red] ip", "UDPBD / UDPFS without a catalog need a fixed IP in [red] ip");
	else if (udp && (net_busy = 1, // Neutrino's smap owns the adapter from here on
	                 !net_dev9() || !load_file(ndir, "smap.irx", NULL, 0) || !load_file(ndir, "ministack.irx", arg, n)))
		fail = L("No se cargaron smap / ministack de neutrino/modules", "smap / ministack from neutrino/modules did not load");
	for (int t = SRC_UDPBD; fail && t <= SRC_UDPFS; t++)
		if (udp & 1u << t) FAIL(t, fail);
	if (fail) return why;
	static char e[2][96];
	if (udp & 1u << SRC_UDPBD) {
		neutrino_ip(ndir, "bsd-udpbd.toml", ip);
		if (!load_file(ndir, "udpbd.irx", NULL, 0)) FAIL(SRC_UDPBD, L("No se cargó udpbd.irx de neutrino/modules", "udpbd.irx from neutrino/modules did not load"));
		else if (!add(SRC_UDPBD, "udpbd0:", 24))
			snprintf(e[0], sizeof(e[0]), L("No respondió el servidor UDPBD (la PS2 es %s, misma red?)", "The UDPBD server did not answer (the PS2 is %s, same network?)"), ip), FAIL(SRC_UDPBD, e[0]);
	}
	if (udp & 1u << SRC_UDPFS) { // udpfs_ioman looks for the server (broadcast, 5 s) as it starts: "udpfs:"
		neutrino_ip(ndir, "bsd-udpfs.toml", ip);
		if (!load_file(ndir, "udpfs_ioman.irx", NULL, 0)) FAIL(SRC_UDPFS, L("No se cargó udpfs_ioman.irx de neutrino/modules", "udpfs_ioman.irx from neutrino/modules did not load"));
		else if (!add(SRC_UDPFS, "udpfs:", 8))
			snprintf(e[1], sizeof(e[1]), L("No respondió el servidor UDPFS (la PS2 es %s, misma red?)", "The UDPFS server did not answer (the PS2 is %s, same network?)"), ip), FAIL(SRC_UDPFS, e[1]);
	}
	return why;
}

// SMB (Neutrino's read-only FHI): the launcher can open files through smb_ioman, but that
// module deliberately has no directory enumeration. TEa.G3 therefore reads a small TEAG3.GAMES
// index from the share; Neutrino opens the listed ISO directly through smb_fhi at launch.
static int smb_ipv4(const char *s)
{
	unsigned value = 0, digits = 0, octets = 0;
	if (!s || !*s) return 0;
	for (; *s; s++) {
		if (*s >= '0' && *s <= '9') {
			value = value * 10 + (unsigned)(*s - '0');
			if (++digits > 3 || value > 255) return 0;
		} else if (*s == '.' && digits && octets < 3) {
			octets++, value = digits = 0;
		} else return 0;
	}
	return octets == 3 && digits > 0;
}

static int smb_field(const char *s, int max, int empty_ok)
{
	int n = s ? strlen(s) : 0;
	if ((!n && !empty_ok) || n > max) return 0;
	for (int i = 0; i < n; i++) {
		unsigned char ch = (unsigned char)s[i];
		if (ch < 0x20 || ch > 0x7e || ch == '"' || ch == '\\') return 0;
	}
	return 1;
}

static int smb_arg_add(char *out, int cap, int *used, const char *key, const char *value)
{
	int n = snprintf(out + *used, cap - *used, "%s=%s", key, value);
	if (n < 0 || n + 1 > cap - *used) return 0;
	*used += n + 1; // SifExecModuleBuffer expects NUL-separated arguments
	return 1;
}

static int smb_write_neutrino_config(const char *ndir, const char *ip, const char *mask, const char *gw,
                                     const char *server, const char *user, const char *password, const char *share)
{
	char path[128], temp[136];
	snprintf(path, sizeof(path), "%s/config/bsd-smb.toml", ndir);
	snprintf(temp, sizeof(temp), "%s.new", path);
	FILE *f = fopen(temp, "wb");
	if (!f) return 0;
	fprintf(f,
	        "# TEa.G3: Neutrino SMB backend; generated from tea_g3/config.ini\n"
	        "name = \"SMB driver\"\n"
	        "depends = [\"i_dev9_hidden\"]\n"
	        "[[module]]\nfile = \"iomanX.irx\"\nenv = [\"LE\"]\n"
	        "[[module]]\nfile = \"fileXio.irx\"\nenv = [\"LE\"]\n"
	        "[[module]]\nfile = \"smap.irx\"\nenv = [\"LE\", \"EE\"]\n"
	        "[[module]]\nfile = \"ministack.irx\"\nargs = [\"ip=%s\"]\nenv = [\"LE\", \"EE\"]\n"
	        "[[module]]\nfile = \"smb_ioman.irx\"\n"
	        "args = [\"ip=%s\", \"mask=%s\", \"gw=%s\", \"srv=%s\", \"user=%s\", \"pass=%s\", \"share=%s\"]\n"
	        "env = [\"LE\"]\n"
	        "[[module]]\nfile = \"smb_fhi.irx\"\n"
	        "args = [\"ip=%s\", \"mask=%s\", \"gw=%s\", \"srv=%s\", \"user=%s\", \"pass=%s\", \"share=%s\"]\n"
	        "env = [\"EE\"]\n",
	        ip, ip, mask, gw, server, user, password, share, ip, mask, gw, server, user, password, share);
	int ok = !ferror(f);
	if (fclose(f) != 0) ok = 0;
	if (!ok) { remove(temp); return 0; }
	if (rename(temp, path)) { remove(temp); return 0; }
	return 1;
}

const char *src_smb_init(const char *ip, const char *mask, const char *gw, const char *server,
                         const char *user, const char *password, const char *share, const char *ndir)
{
	static const char *bad_net = "SMB needs a fixed PS2 IPv4 address, mask and gateway in [red]";
	static const char *bad_cfg = "SMB settings are invalid: check [smb] server/share/user/password (ASCII limits apply)";
	static const char *missing = "SMB modules or Neutrino configuration are missing";
	static const char *full = "Too many game sources are enabled";
	static const char *bad_toml = "Could not write neutrino/config/bsd-smb.toml";
	char min_args[64], smb_args[512];
	int min_len = 0, smb_len = 0;

	if (!ip || !strcasecmp(ip, "dhcp") || !smb_ipv4(ip) || !smb_ipv4(mask) ||
	    (gw && *gw && !smb_ipv4(gw))) return src_err[SRC_SMB] = bad_net;
	if (!server || !smb_ipv4(server) || !smb_field(user, 31, 1) ||
	    !smb_field(password, 31, 1) || !smb_field(share, 63, 0))
		return src_err[SRC_SMB] = bad_cfg;
	if (nsrc >= SRC_MAX) return src_err[SRC_SMB] = full;
	if (!smb_write_neutrino_config(ndir, ip, mask, gw && *gw ? gw : "0.0.0.0",
	                               server, user && *user ? user : "guest", password ? password : "", share))
		return src_err[SRC_SMB] = bad_toml;

	if (!smb_arg_add(min_args, sizeof(min_args), &min_len, "ip", ip) ||
	    !smb_arg_add(smb_args, sizeof(smb_args), &smb_len, "ip", ip) ||
	    !smb_arg_add(smb_args, sizeof(smb_args), &smb_len, "mask", mask) ||
	    !smb_arg_add(smb_args, sizeof(smb_args), &smb_len, "gw", gw && *gw ? gw : "0.0.0.0") ||
	    !smb_arg_add(smb_args, sizeof(smb_args), &smb_len, "srv", server) ||
	    !smb_arg_add(smb_args, sizeof(smb_args), &smb_len, "user", user && *user ? user : "guest") ||
	    !smb_arg_add(smb_args, sizeof(smb_args), &smb_len, "pass", password ? password : "") ||
	    !smb_arg_add(smb_args, sizeof(smb_args), &smb_len, "share", share))
		return src_err[SRC_SMB] = bad_cfg;

	if (!net_dev9() || !load_file(ndir, "smap.irx", NULL, 0) ||
	    !load_file(ndir, "ministack.irx", min_args, min_len) ||
	    !load_file(ndir, "smb_ioman.irx", smb_args, smb_len))
		return src_err[SRC_SMB] = missing;

	net_busy = 1; // SMB's tiny TCP/IP stack owns the adapter; do not start TEa.G3's lwIP stack as well.
	src[nsrc].type = SRC_SMB;
	src[nsrc].unit = 0;
	strcpy(src[nsrc].root, "smb:");
	printf("sources: smb share %s at %s (catalog TEAG3.GAMES)\n", share, server);
	nsrc++;
	src_err[SRC_SMB] = NULL;
	return NULL;
}

// phase 17: the game stage's ministack (Neutrino boots it, no -qb) takes the address the launcher has, fixed or the
// DHCP lease: ministack itself has no DHCP
void src_udpfs_ip(const char *ndir, const char *ip)
{
	if (*ip) neutrino_ip(ndir, "bsd-udpfs.toml", ip);
}

// HD Loader metadata, matching upstream OPL's hddGetHDLGamelist() calculation.
typedef struct hdl_scan_record {
	struct hdl_scan_record *next;
	char part[APA_IDMAX + 1], title[161], startup[61];
	u32 start_sector, total_size_kb;
} hdl_scan_record;

int src_hdl_scan(void (*cb)(void *ctx, const char *part, const char *title, const char *startup, u32 start_sector, u32 total_size_kb, int is_cd), void *ctx)
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
		snprintf(g->part, sizeof(g->part), "%s", de.name);
		snprintf(g->title, sizeof(g->title), "%s", (const char *)h + 8);
		snprintf(g->startup, sizeof(g->startup), "%s", (const char *)h + 172);
		g->start_sector = a.lba;
		g->is_cd = ((u8)h[236] == 0x12 || (u8)h[236] == 0x13); // SCECdPS2CD / SCECdPS2CDDA
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
			cb(ctx, g->part, g->title, g->startup, g->start_sector, g->total_size_kb, g->is_cd);
			n++;
		}
		free(g);
		g = next;
	}
	return n;
}

void src_mmce_game(const char *startup) // every MMCE: devctl 0x1 ping, 0x8 set game ID, poll 0x2 until not busy (nhddl mmce.c)
{
	char dev[] = "mmce0:";
	for (int u = 0; u < 2; u++) {
		dev[4] = '0' + u;
		if (fileXioDevctl(dev, 0x1, NULL, 0, NULL, 0) < 0 || fileXioDevctl(dev, 0x8, (void *)startup, strlen(startup) + 1, NULL, 0) < 0) continue;
		for (int t = 0; t < 30 && (fileXioDevctl(dev, 0x2, NULL, 0, NULL, 0) & 1); t++) usleep(500000);
	}
}
