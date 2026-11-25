// Virtual memory cards (phase 14). Layout of a fresh 8 MB card as OPL's genvmc writes it (modules/vmc/genvmc/genvmc.c,
// reference only, docs/sources.md): 1 KB clusters of 2 pages, 16-page blocks, 8192 clusters; superblock in cluster 0,
// the one indirect FAT cluster at 8 (blocksize / 2), 32 FAT clusters at 9..40, root directory at 41 = alloc_offset;
// the last two blocks are the backup blocks, everything unused is erased (0xFF). Neutrino's mc_emu computes the ECC
// itself, so the image has none (8 388 608 bytes).
// Reading: superblock -> ifc_list -> FAT chains; directory entries are 512 bytes, counted by the directory's entry in
// its parent (the root's by its own ".").
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include "vmc.h"

#define CSIZE 1024
#define NCL (VMC_SIZE / CSIZE) // 8192 clusters
#define PER (CSIZE / 4)        // FAT entries per cluster
#define IFC 8
#define FAT0 9
#define FATN 32                // (NCL * 4 - 1) / CSIZE + 1
#define ALLOC (FAT0 + FATN)    // 41
#define ALLOC_END (((NCL / 8) - 2) * 8 - ALLOC) // 8135: up to the backup blocks

static void put16(unsigned char *p, unsigned v) { p[0] = v, p[1] = v >> 8; }
static void put32(unsigned char *p, unsigned v) { p[0] = v, p[1] = v >> 8, p[2] = v >> 16, p[3] = v >> 24; }
static unsigned get32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }
static unsigned get16(const unsigned char *p) { return p[0] | p[1] << 8; }

static void now(unsigned char *t) // sceMcStDateTime from the C clock (cosmetic: the root directory's dates)
{
	time_t s = time(NULL);
	struct tm *tm = gmtime(&s);
	memset(t, 0, 8);
	if (!tm) { t[4] = 1, t[5] = 1, put16(t + 6, 2000); return; }
	t[1] = tm->tm_sec, t[2] = tm->tm_min, t[3] = tm->tm_hour, t[4] = tm->tm_mday, t[5] = tm->tm_mon + 1;
	put16(t + 6, tm->tm_year + 1900);
}

static void dir_entry(unsigned char *e, unsigned mode, const char *name, const unsigned char *t)
{
	memset(e, 0, 512);
	put16(e, mode);
	put32(e + 4, 2);   // length: the root holds "." and ".."
	memcpy(e + 8, t, 8);  // created
	memcpy(e + 24, t, 8); // modified; cluster 0, dir_entry 0
	strcpy((char *)e + 64, name);
}

static void cluster(unsigned c, unsigned char *b, const unsigned char *t)
{
	memset(b, 0xFF, CSIZE);
	if (c == 0) { // superblock (genvmc MCDevInfo, 384 bytes)
		memset(b, 0, 384);
		memcpy(b, "Sony PS2 Memory Card Format 1.2.0.0", 35);
		put16(b + 0x28, 512), put16(b + 0x2A, 2), put16(b + 0x2C, 16);
		put32(b + 0x30, NCL), put32(b + 0x34, ALLOC), put32(b + 0x38, ALLOC_END), put32(b + 0x3C, 0);
		put32(b + 0x40, NCL / 8 - 1), put32(b + 0x44, NCL / 8 - 2);
		for (int i = 0; i < 32; i++) put32(b + 0x50 + 4 * i, i ? 0xFFFFFFFF : IFC), put32(b + 0xD0 + 4 * i, 0xFFFFFFFF);
		b[0x150] = 2, b[0x151] = 0x2B; // PS2 card; the flags genvmc and Neutrino's mc_emu report
		put32(b + 0x154, CSIZE), put32(b + 0x158, PER), put32(b + 0x15C, 8), put32(b + 0x160, 1);
		put32(b + 0x170, NCL / 1000 * 1000 + 1); // max_allocatable_clusters as genvmc computes it (8001)
		put32(b + 0x17C, 0xFFFFFFFF);
	} else if (c == IFC) {
		memset(b, 0, CSIZE);
		for (int i = 0; i < FATN; i++) put32(b + 4 * i, FAT0 + i);
	} else if (c >= FAT0 && c < FAT0 + FATN) {
		memset(b, 0, CSIZE);
		for (int i = 0; i < PER; i++) {
			unsigned e = (c - FAT0) * PER + i;
			if (e < ALLOC_END) put32(b + 4 * i, e ? 0x7FFFFFFF : 0xFFFFFFFF); // free; the root's one cluster ends
		}
	} else if (c == ALLOC) {
		dir_entry(b, 0x8427, ".", t);       // exists | 0400 | subdir | rwx
		dir_entry(b + 512, 0xA426, "..", t); // exists | hidden | 0400 | subdir | wx
	}
}

int vmc_create(const char *path)
{
    char temp[160];
    FILE *f = NULL, *existing;
    unsigned char t[8], *buf = NULL;
    int ok = 0;
    if (!path || !*path || snprintf(temp, sizeof(temp), "%s.tmp", path) >= (int)sizeof(temp))
        return 0;
    /* Never truncate a pre-existing card, even if the caller's existence check raced. */
    existing = fopen(path, "rb");
    if (existing) { fclose(existing); return 0; }
    buf = malloc(32 * CSIZE);
    if (!buf) return 0;
    remove(temp);
    f = fopen(temp, "wb");
    if (!f) { free(buf); return 0; }
    now(t);
    ok = 1;
    for (unsigned c = 0; ok && c < NCL; c += 32) {
        for (unsigned k = 0; k < 32; k++) cluster(c + k, buf + k * CSIZE, t);
        ok = fwrite(buf, 32 * CSIZE, 1, f) == 1;
    }
    if (fclose(f) != 0) ok = 0;
    free(buf);
    if (ok) {
        vmc_ent check[2];
        if (vmc_list(temp, check, 2) != 0) ok = 0;
    }
    if (ok) {
        existing = fopen(path, "rb");
        if (existing) { fclose(existing); ok = 0; }
    }
    if (ok && rename(temp, path) == 0) return 1;
    remove(temp);
    return 0;
}

typedef struct {
    FILE *f;
    unsigned csize, alloc, ifc[32], fat_cl, fat_ok;
    unsigned char b[CSIZE], fat[CSIZE];
} card;

static int rd(card *v, unsigned c, unsigned char *b)
{
    unsigned long offset;
    if (!v || !v->f || !b || c >= NCL || !v->csize ||
        c > 0x7fffffffu / v->csize) return 0;
    offset = (unsigned long)c * v->csize;
    return fseek(v->f, (long)offset, SEEK_SET) == 0 &&
           fread(b, v->csize, 1, v->f) == 1;
}

static int open_card(card *v, const char *path)
{
    unsigned char s[512];
    long bytes;
    if (!v || !path || !*path) return 0;
    memset(v, 0, sizeof(*v));
    v->fat_cl = 0xffffffffu;
    v->f = fopen(path, "rb");
    if (!v->f) return 0;
    if (fseek(v->f, 0, SEEK_END) != 0 || (bytes = ftell(v->f)) < VMC_SIZE ||
        fseek(v->f, 0, SEEK_SET) != 0 || fread(s, 512, 1, v->f) != 1 ||
        memcmp(s, "Sony PS2 Memory Card Format ", 28) ||
        get16(s + 0x28) != 512 || get16(s + 0x2A) != 2 ||
        get16(s + 0x2C) != 16 || get32(s + 0x30) != NCL ||
        get32(s + 0x34) != ALLOC || get32(s + 0x154) != CSIZE) {
        fclose(v->f), v->f = NULL;
        return 0;
    }
    v->csize = CSIZE, v->alloc = get32(s + 0x34);
    for (int i = 0; i < 32; i++) {
        v->ifc[i] = get32(s + 0x50 + 4 * i);
        if (v->ifc[i] != 0xffffffffu &&
            (v->ifc[i] < FAT0 || v->ifc[i] >= FAT0 + FATN)) {
            fclose(v->f), v->f = NULL;
            return 0;
        }
    }
    if (v->ifc[0] == 0xffffffffu) {
        fclose(v->f), v->f = NULL;
        return 0;
    }
    return 1;
}

static unsigned next(card *v, unsigned rel)
{
    unsigned per, idx, ifc_idx, ifc_cluster, fc, e, target;
    if (!v || rel >= ALLOC_END) return 0xffffffffu;
    per = v->csize / 4;
    idx = rel / per;
    ifc_idx = idx / per;
    if (ifc_idx >= 32) return 0xffffffffu;
    ifc_cluster = v->ifc[ifc_idx];
    if (ifc_cluster == 0xffffffffu || ifc_cluster >= NCL || !rd(v, ifc_cluster, v->b)) return 0xffffffffu;
    fc = get32(v->b + 4 * (idx % per));
    if (fc == 0xffffffffu || fc < FAT0 || fc >= FAT0 + FATN) return 0xffffffffu;
    if (!v->fat_ok || v->fat_cl != fc) {
        if (!rd(v, fc, v->fat)) return 0xffffffffu;
        v->fat_cl = fc, v->fat_ok = 1;
    }
    e = get32(v->fat + 4 * (rel % per));
    if (e == 0xffffffffu || !(e & 0x80000000u)) return 0xffffffffu;
    target = e & 0x7fffffffu;
    return target < ALLOC_END ? target : 0xffffffffu;
}

/* 1 = entry read, 0 = chain ended, -1 = corrupt / I/O error. */
static int dir_get(card *v, unsigned rel, unsigned k, unsigned char *e)
{
    unsigned steps = k / 2;
    if (!v || !e || rel >= ALLOC_END || steps >= ALLOC_END) return -1;
    for (unsigned n = 0; n < steps; n++) {
        unsigned following = next(v, rel);
        if (following == 0xffffffffu) return 0;
        rel = following;
    }
    if (v->alloc >= NCL || rel >= NCL - v->alloc || !rd(v, v->alloc + rel, v->b))
        return -1;
    memcpy(e, v->b + (k & 1) * 512, 512);
    return 1;
}

static void to_ent(const unsigned char *e, vmc_ent *o)
{
    memcpy(o->name, e + 64, 32);
    o->name[32] = 0;
    o->mode = get16(e);
    memcpy(o->modified, e + 24, 8);
    o->cluster = get32(e + 16), o->length = get32(e + 4);
}

/* Directory entry count includes . and ..; return -1 on any short/corrupt chain. */
static int list(card *v, unsigned rel, unsigned count, vmc_ent *out, int max)
{
    unsigned char e[512];
    unsigned limit = NCL * 2;
    int n = 0;
    int status;
    if (!v || !out || max <= 0 || rel >= ALLOC_END) return -1;
    status = dir_get(v, rel, 0, e);
    if (status != 1) return -1;
    if (!count) count = get32(e + 4);
    if (count < 2 || count > limit) return -1;
    for (unsigned k = 2; k < count; k++) {
        status = dir_get(v, rel, k, e);
        if (status != 1) return -1;
        if (!(get16(e) & 0x8000u)) continue;
        if (!memchr(e + 64, 0, 32)) return -1;
        if (get32(e + 16) >= ALLOC_END && (get16(e) & 0x20u)) return -1;
        if (n < max) to_ent(e, &out[n++]);
    }
    return n;
}

int vmc_list(const char *path, vmc_ent *out, int max)
{
    card *v;
    int n = -1;
    if (!out || max <= 0) return -1;
    v = malloc(sizeof(card));
    if (v && open_card(v, path)) {
        n = list(v, 0, 0, out, max);
        fclose(v->f);
    }
    free(v);
    return n;
}

void *vmc_read(const char *path, const char *file, int *size)
{
    char dir[33];
    const char *slash;
    card *v = NULL;
    vmc_ent *e = NULL;
    unsigned char *data = NULL;
    unsigned char *visited = NULL;
    int n, d = -1, f = -1;
    if (!path || !file || !size || !(slash = strchr(file, '/')) ||
        slash == file || slash - file > 32 || !slash[1]) return NULL;
    v = malloc(sizeof(card));
    e = malloc(sizeof(vmc_ent) * 64);
    visited = calloc((ALLOC_END + 7) / 8, 1);
    if (!v || !e || !visited || !open_card(v, path)) goto out;
    snprintf(dir, sizeof(dir), "%.*s", (int)(slash - file), file);
    n = list(v, 0, 0, e, 64);
    if (n < 0) goto close;
    for (int i = 0; i < n; i++)
        if ((e[i].mode & 0x20u) && !strcmp(e[i].name, dir)) { d = i; break; }
    if (d < 0 || e[d].cluster >= ALLOC_END || e[d].length < 2) goto close;
    n = list(v, e[d].cluster, e[d].length, e, 64);
    if (n < 0) goto close;
    for (int i = 0; i < n; i++)
        if (!(e[i].mode & 0x20u) && !strcmp(e[i].name, slash + 1)) { f = i; break; }
    if (f < 0 || !e[f].length || e[f].length > (2u << 20) || e[f].cluster >= ALLOC_END) goto close;
    {
        unsigned expected = (e[f].length + v->csize - 1) / v->csize;
        unsigned rel = e[f].cluster, got = 0;
        size_t alloc_size = ((size_t)expected * v->csize + 63u) & ~(size_t)63u;
        if (!expected || expected > ALLOC_END) goto close;
        data = memalign(64, alloc_size);
        if (!data) goto close;
        for (unsigned k = 0; k < expected; k++) {
            unsigned amount = e[f].length - got;
            if (amount > v->csize) amount = v->csize;
            if (rel >= ALLOC_END || (visited[rel >> 3] & (1u << (rel & 7))) ||
                !rd(v, v->alloc + rel, v->b)) {
                free(data), data = NULL;
                goto close;
            }
            visited[rel >> 3] |= (unsigned char)(1u << (rel & 7));
            memcpy(data + got, v->b, amount);
            got += amount;
            if (k + 1 < expected) {
                rel = next(v, rel);
                if (rel == 0xffffffffu) { free(data), data = NULL; goto close; }
            }
        }
        if (got != e[f].length) { free(data), data = NULL; goto close; }
        *size = (int)got;
    }
close:
    if (v && v->f) fclose(v->f);
out:
    free(visited), free(v), free(e);
    return data;
}

#if defined(SELFTEST) && !defined(SELFTEST_LIB) // host check: `make test`; VMC_CHECK=<card.bin> also lists a card made elsewhere (mymcplus, PCSX2)
#include <assert.h>
int main(int argc, char **argv)
{
	const char *p = "/tmp/vmc_selftest.bin";
	/* This fixed path belongs to the host selftest; clear only its own prior output before testing no-overwrite. */
	remove(p);
	assert(vmc_create(p));
	FILE *f = fopen(p, "rb");
	fseek(f, 0, SEEK_END);
	assert(ftell(f) == VMC_SIZE);
	fclose(f);
	vmc_ent e[8];
	assert(vmc_list(p, e, 8) == 0); // formatted and empty
    /* Exercise the IFC -> FAT -> directory -> file path on an actual VMC image. */
    {
        FILE *card = fopen(p, "r+b");
        unsigned char word[4], folder[512], directory[1024], file_entry[512], data[CSIZE];
        int size = 0;
        void *payload;
        assert(card);
        put32(word, 3);
        assert(fseek(card, ALLOC * CSIZE + 4, SEEK_SET) == 0 && fwrite(word, 1, 4, card) == 4);
        put32(word, 0x80000001u);
        assert(fseek(card, FAT0 * CSIZE, SEEK_SET) == 0 && fwrite(word, 1, 4, card) == 4);
        memset(folder, 0, sizeof(folder));
        put16(folder, 0x8427); put32(folder + 4, 3); put32(folder + 16, 2);
        memcpy(folder + 64, "TESTDIR", 7);
        assert(fseek(card, (ALLOC + 1) * CSIZE, SEEK_SET) == 0 && fwrite(folder, 1, sizeof(folder), card) == sizeof(folder));
        put32(word, 0x80000003u);
        assert(fseek(card, FAT0 * CSIZE + 8, SEEK_SET) == 0 && fwrite(word, 1, 4, card) == 4);
        memset(directory, 0, sizeof(directory));
        put16(directory, 0x8427); put32(directory + 4, 3); memcpy(directory + 64, ".", 1);
        put16(directory + 512, 0xA426); put32(directory + 516, 3); memcpy(directory + 576, "..", 2);
        assert(fseek(card, (ALLOC + 2) * CSIZE, SEEK_SET) == 0 && fwrite(directory, 1, sizeof(directory), card) == sizeof(directory));
        memset(file_entry, 0, sizeof(file_entry));
        put16(file_entry, 0x8001); put32(file_entry + 4, 5); put32(file_entry + 16, 4);
        memcpy(file_entry + 64, "icon.sys", 8);
        assert(fseek(card, (ALLOC + 3) * CSIZE, SEEK_SET) == 0 && fwrite(file_entry, 1, sizeof(file_entry), card) == sizeof(file_entry));
        memset(data, 0xff, sizeof(data)); memcpy(data, "PS2D!", 5);
        assert(fseek(card, (ALLOC + 4) * CSIZE, SEEK_SET) == 0 && fwrite(data, 1, sizeof(data), card) == sizeof(data));
        assert(fclose(card) == 0);
        assert(vmc_list(p, e, 8) == 1 && !strcmp(e[0].name, "TESTDIR"));
        payload = vmc_read(p, "TESTDIR/icon.sys", &size);
        assert(payload && size == 5 && !memcmp(payload, "PS2D!", 5));
        free(payload);
    }
	assert(vmc_list("/tmp/does-not-exist.bin", e, 8) == -1);
    /* Creating over an existing path must never truncate a user's existing card. */
    {
        FILE *keep = fopen("/tmp/vmc_keep.bin", "wb");
        char check[8] = {0};
        assert(keep && fwrite("KEEP", 1, 4, keep) == 4 && fclose(keep) == 0);
        assert(!vmc_create("/tmp/vmc_keep.bin"));
        keep = fopen("/tmp/vmc_keep.bin", "rb");
        assert(keep && fread(check, 1, 4, keep) == 4 && fclose(keep) == 0);
        assert(!memcmp(check, "KEEP", 4));
        remove("/tmp/vmc_keep.bin");
    }
    /* Short and structurally invalid card images are rejected before directory traversal. */
    {
        FILE *bad = fopen("/tmp/vmc_short.bin", "wb");
        assert(bad && fwrite("Sony", 1, 4, bad) == 4 && fclose(bad) == 0);
        assert(vmc_list("/tmp/vmc_short.bin", e, 8) == -1);
        remove("/tmp/vmc_short.bin");
    }
    {
        FILE *bad = fopen(p, "r+b");
        unsigned char word[4] = {0xff, 0xff, 0xff, 0x7f};
        assert(bad && fseek(bad, 0x34, SEEK_SET) == 0 && fwrite(word, 1, 4, bad) == 4 && fclose(bad) == 0);
        assert(vmc_list(p, e, 8) == -1);
    }
	assert(!vmc_read(p, "BASLUS-21376/icon.sys", &argc));
	if (argc > 1) {
		vmc_ent big[64];
		int n = vmc_list(argv[1], big, 64);
		printf("%s: %d entries\n", argv[1], n);
		for (int i = 0; i < n; i++) {
			printf("  %s mode %04x length %u %02d/%02d/%04d\n", big[i].name, big[i].mode, big[i].length, big[i].modified[4],
			       big[i].modified[5], get16(big[i].modified + 6));
			if (argc > 2 && big[i].mode & 0x20) {
				char path[80];
				int sz = 0;
				snprintf(path, sizeof(path), "%s/%s", big[i].name, argv[2]);
				void *d = vmc_read(argv[1], path, &sz);
				printf("    %s: %d bytes%s\n", path, d ? sz : -1, d && sz > 4 && !memcmp(d, "PS2D", 4) ? " (PS2D icon.sys)" : "");
				free(d);
			}
		}
	}
	puts("vmc selftest ok");
	return 0;
}
#endif
