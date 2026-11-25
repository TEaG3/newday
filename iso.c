// PS2 game ISO identification: serial from SYSTEM.CNF (ISO9660), title from the file name.
// ISO9660: primary volume descriptor at sector 16; root directory record at offset 156 (extent LBA at +2, size at
// +10, little-endian); directory records: length at +0, name length at +32, name at +33 ("SYSTEM.CNF;1").
// SYSTEM.CNF: "BOOT2 = cdrom0:\SLUS_209.46;1" (PS2) or "BOOT = cdrom:\SLUS_007.47;1" (PS1). Spec: phase-6-games,
// phase-11-library.
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include "iso.h"

#define SECTOR 2048

static unsigned le32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24; }

// SYSTEM.CNF can sit past 2 GB (Persona 4: LBA 1 761 296 = 3.6 GB) and the EE's long is 32 bits, so the caller
// supplies a 64-bit-capable reader (fileXioLseek64 on the PS2, stdio on the host)
int iso_find(iso_read_fn read_at, void *f, const char *path, unsigned *lba_out, unsigned *size_out)
{
    static unsigned char buf[SECTOR] __attribute__((aligned(64)));
    unsigned lba, size;
    const char *component;
    if (!read_at || !path || !lba_out || !size_out) return 0;
    if (!read_at(f, 16, buf, SECTOR) || buf[0] != 1 ||
        memcmp(buf + 1, "CD001", 5) || buf[6] != 1) return 0;
    /* Primary volume descriptor's root record must fit and describe a directory. */
    if (buf[156] < 34 || buf[156] > SECTOR - 156 ||
        buf[156 + 32] != 1 || buf[156 + 33] != 0 ||
        !(buf[156 + 25] & 0x02)) return 0;
    lba = le32(buf + 156 + 2), size = le32(buf + 156 + 10);
    if (!size || (unsigned long long)lba + ((unsigned long long)size - 1) / SECTOR > 0xffffffffu) return 0;

    while (*path == '\\' || *path == '/') path++;
    if (!*path) return 0;
    component = path;
    for (;;) {
        unsigned n = (unsigned)strcspn(component, "\\/");
        unsigned off = 0, found_lba = 0, found_size = 0;
        int found = 0, found_dir = 0;
        int more;
        if (!n || n > 255) return 0;
        more = component[n] != 0;
        while (off < size && !found) {
            unsigned read_size = size - off;
            unsigned p = 0;
            if (read_size > SECTOR) read_size = SECTOR;
            memset(buf, 0, sizeof(buf));
            if ((unsigned long long)lba + off / SECTOR > 0xffffffffu ||
                !read_at(f, lba + off / SECTOR, buf, read_size)) return 0;
            while (p < read_size && buf[p]) {
                unsigned record_size = buf[p];
                unsigned name_size;
                const unsigned char *name;
                int match;
                if (record_size < 34 || record_size > read_size - p) return 0;
                name_size = buf[p + 32];
                if (name_size > record_size - 33) return 0;
                name = buf + p + 33;
                match = name_size == n && !strncasecmp((const char *)name, component, n);
                if (!match && name_size > n && name[n] == ';' &&
                    !strncasecmp((const char *)name, component, n)) match = 1;
                if (match) {
                    found_lba = le32(buf + p + 2);
                    found_size = le32(buf + p + 10);
                    found_dir = (buf[p + 25] & 0x02) != 0;
                    if (!found_size ||
                        (unsigned long long)found_lba + ((unsigned long long)found_size - 1) / SECTOR > 0xffffffffu) return 0;
                    found = 1;
                    break;
                }
                p += record_size;
            }
            off += read_size;
        }
        if (!found || (more && !found_dir)) return 0;
        if (!more) {
            *lba_out = found_lba, *size_out = found_size;
            return 1;
        }
        lba = found_lba, size = found_size;
        component += n + 1;
        if (!*component) return 0;
    }
}

int iso_cnf(iso_read_fn read_at, void *f, char *out, unsigned max)
{
    static unsigned char buf[SECTOR * 2] __attribute__((aligned(64)));
    unsigned lba, size;
    if (!read_at || !out || !max || !iso_find(read_at, f, "SYSTEM.CNF", &lba, &size) || !size) return 0;
    if (size > max - 1) size = max - 1;
    if (size > sizeof(buf)) size = sizeof(buf);
    if (!size || !read_at(f, lba, buf, size)) return 0;
    memcpy(out, buf, size);
    out[size] = 0;
    return 1;
}

int iso_serial(iso_read_fn read_at, void *f, int ps1, char *out) // out: >= 16 bytes, "SLUS_209.46"; 1 if found
{
    static char cnf[SECTOR * 2];
    return out && iso_cnf(read_at, f, cnf, sizeof(cnf)) && cnf_boot(cnf, ps1, out, NULL, 0);
}

int cnf_boot(const char *cnf, int ps1, char *file, char *raw, int raw_n)
{
    const char *b;
    if (!cnf || !file || (raw && raw_n <= 0)) return 0;
    b = cnf;
    for (;;) {
        const char *line_end;
        int key_len = ps1 ? 4 : 5;
        if (!(b = strstr(b, "BOOT"))) return 0;
        if (b != cnf && b[-1] != '\n') { b += 4; continue; }
        if (ps1) {
            if (!(b[4] == '=' || b[4] == ' ' || b[4] == '\t')) { b += 4; continue; }
        } else {
            if (b[4] != '2' || !(b[5] == '=' || b[5] == ' ' || b[5] == '\t')) { b += 4; continue; }
        }
        line_end = strpbrk(b, "\r\n");
        if (!line_end) line_end = b + strlen(b);
        const char *eq = memchr(b + key_len, '=', (size_t)(line_end - (b + key_len)));
        if (!eq) {
            if (!*line_end) return 0;
            b = line_end + 1;
            continue;
        }
        const char *value = eq + 1;
        while (value < line_end && (*value == ' ' || *value == '\t')) value++;
        while (line_end > value && isspace((unsigned char)line_end[-1])) line_end--;
        if (value >= line_end) return 0;
        if (raw) {
            size_t raw_len = (size_t)(line_end - value);
            if ((size_t)raw_n <= raw_len) return 0;
            memcpy(raw, value, raw_len), raw[raw_len] = 0;
        }
        const char *name = value;
        for (const char *p = value; p < line_end; p++)
            if (*p == '\\' || *p == '/' || *p == ':') name = p + 1;
        size_t name_len = 0;
        while (name + name_len < line_end && name[name_len] != ';') name_len++;
        if (!name_len || name_len > 15) return 0;
        memcpy(file, name, name_len);
        file[name_len] = 0;
        return 1;
    }
}

void serial_dash(const char *in, char *out) // "SLUS_209.46" -> "SLUS-20946" (cover and save names)
{
	for (; *in; in++)
		if (*in == '_') *out++ = '-';
		else if (*in != '.') *out++ = *in;
	*out = 0;
}

int name_serial(const char *file, char *out) // OPL naming "SLUS_209.46.Title.iso" -> "SLUS_209.46"; 1 if present
{
	if (strlen(file) < 12 || file[4] != '_' || file[8] != '.' || file[11] != '.') return 0;
	for (int i = 0; i < 11; i++)
		if (i != 4 && i != 8 && !isalnum((unsigned char)file[i])) return 0;
	memcpy(out, file, 11);
	out[11] = 0;
	return 1;
}

void iso_title(const char *file, char *out, int n) // "SLUS_209.46.God of War.iso" -> "God of War"
{
	const char *t = file;
	char tmp[16];
	if (name_serial(t, tmp)) t += 12; // OPL "SERIAL.Title.iso" naming
	snprintf(out, n, "%s", t);
	char *dot = strrchr(out, '.');
	if (dot && (!strcasecmp(dot, ".iso") || !strcasecmp(dot, ".vcd") || !strcasecmp(dot, ".elf") || !strcasecmp(dot, ".zso"))) *dot = 0;
}

static int cat_serial_valid(const char *s)
{
    if (!s || !strcmp(s, "-")) return 1;
    if (strlen(s) != 11) return 0;
    for (int i = 0; i < 11; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (i == 4) { if (ch != '_') return 0; }
        else if (i == 8) { if (ch != '.') return 0; }
        else if (!isalnum(ch)) return 0;
    }
    return 1;
}

static int cat_hash_valid(const char *s)
{
    if (!s || !strcmp(s, "-")) return 1;
    if (strlen(s) != 32) return 0;
    for (int i = 0; i < 32; i++)
        if (!isxdigit((unsigned char)s[i])) return 0;
    return 1;
}

static int cat_utf8_valid(const unsigned char *s)
{
    while (*s) {
        unsigned char a = *s++;
        if (a < 0x80) continue;
        if (a >= 0xc2 && a <= 0xdf) {
            if ((s[0] & 0xc0) != 0x80) return 0;
            s++;
        } else if (a == 0xe0) {
            if (s[0] < 0xa0 || s[0] > 0xbf || (s[1] & 0xc0) != 0x80) return 0;
            s += 2;
        } else if ((a >= 0xe1 && a <= 0xec) || (a >= 0xee && a <= 0xef)) {
            if ((s[0] & 0xc0) != 0x80 || (s[1] & 0xc0) != 0x80) return 0;
            s += 2;
        } else if (a == 0xed) {
            if (s[0] < 0x80 || s[0] > 0x9f || (s[1] & 0xc0) != 0x80) return 0;
            s += 2;
        } else if (a == 0xf0) {
            if (s[0] < 0x90 || s[0] > 0xbf || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80) return 0;
            s += 3;
        } else if (a >= 0xf1 && a <= 0xf3) {
            if ((s[0] & 0xc0) != 0x80 || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80) return 0;
            s += 3;
        } else if (a == 0xf4) {
            if (s[0] < 0x80 || s[0] > 0x8f || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80) return 0;
            s += 3;
        } else return 0;
    }
    return 1;
}

static int cat_path_valid(const char *path)
{
    const char *part;
    size_t len;
    int app;
    if (!path || !(len = strlen(path)) || len >= 160 || !cat_utf8_valid((const unsigned char *)path) || path[0] == '/' ||
        path[len - 1] == '/' || strchr(path, '\\') || strchr(path, ':')) return 0;
    app = !strncmp(path, "APPS/", 5);
    if (!app && strncmp(path, "DVD/", 4) && strncmp(path, "CD/", 3)) return 0;
    for (const unsigned char *p = (const unsigned char *)path; *p; p++) {
        if (*p < 0x20 || *p == 0x7f || *p == '?' || *p == '#') return 0;
        if (*p == '%') {
            int h1, h2;
            if (!p[1] || !p[2] || !isxdigit(p[1]) || !isxdigit(p[2])) return 0;
            h1 = isdigit(p[1]) ? p[1] - '0' : tolower(p[1]) - 'a' + 10;
            h2 = isdigit(p[2]) ? p[2] - '0' : tolower(p[2]) - 'a' + 10;
            int decoded = (h1 << 4) | h2;
            if (decoded == '/' || decoded == '\\' || decoded == '.' ||
                decoded == ':' || decoded < 0x20 || decoded == 0x7f) return 0;
            p += 2;
        }
    }
    part = path;
    for (const char *p = path;; p++) {
        if (*p == '/' || !*p) {
            size_t n = (size_t)(p - part);
            if (!n || (n == 1 && part[0] == '.') ||
                (n == 2 && part[0] == '.' && part[1] == '.')) return 0;
            if (!*p) break;
            part = p + 1;
        }
    }
    if (app) return len >= 4 && !strcasecmp(path + len - 4, ".elf");
    return (len >= 4 && !strcasecmp(path + len - 4, ".iso")) ||
           (len >= 4 && !strcasecmp(path + len - 4, ".zso"));
}

int catalog_next(char **p, cat_ent *e)
{
    if (!p || !*p || !e) return 0;
    while (**p) {
        char *line = *p, *nl = strpbrk(line, "\r\n"), *field[5];
        char *end;
        unsigned long long size;
        int n;
        if (nl) {
            *nl = 0;
            end = nl + 1;
            while (*end == '\r' || *end == '\n') end++;
            *p = end;
        } else {
            *p = line + strlen(line);
        }
        n = strlen(line);
        if (!n || line[0] == '#') continue;
        field[0] = line;
        for (int i = 1; i < 5; i++) {
            char *tab = strchr(field[i - 1], '\t');
            if (!tab) { field[0] = NULL; break; }
            *tab = 0;
            field[i] = tab + 1;
        }
        if (!field[0] || strchr(field[4], '\t')) continue;
        if (!*field[0] || !*field[1] || !*field[2] || !*field[3] || !*field[4]) continue;
        if (!cat_path_valid(field[0]) || strlen(field[4]) >= 64 ||
            !cat_serial_valid(field[2]) || !cat_hash_valid(field[3])) continue;
        errno = 0;
        size = strtoull(field[1], &end, 10);
        if (errno || end == field[1] || *end || size == 0 || size > 0xffffffffu) continue;
        e->path = field[0], e->size = size, e->serial = field[2], e->hash = field[3], e->title = field[4];
        return 1;
    }
    return 0;
}

#if defined(SELFTEST) && !defined(SELFTEST_LIB) // host check: `make test` (SELFTEST_LIB: linked into another's)
#include <assert.h>
#include <stdlib.h>
static int read_file(void *f, unsigned lba, void *buf, unsigned n)
{
	return fseek(f, (long)lba * SECTOR, SEEK_SET) == 0 && fread(buf, 1, n, f) == n;
}
#define iso_serial(f, s) iso_serial(read_file, f, 0, s)
int main(void)
{
	// minimal ISO: PVD at 16, root dir at 18 with SYSTEM.CNF -> sector 20
	static unsigned char iso[SECTOR * 21];
	unsigned char *pvd = iso + 16 * SECTOR, *root = iso + 18 * SECTOR, *rec;
	pvd[0] = 1; memcpy(pvd + 1, "CD001", 5); pvd[6] = 1;
	pvd[156] = 34; pvd[156 + 2] = 18; pvd[156 + 10] = SECTOR & 255; pvd[156 + 11] = SECTOR >> 8;
    pvd[156 + 25] = 2; pvd[156 + 32] = 1; pvd[156 + 33] = 0;
	rec = root; rec[0] = 34; rec[32] = 1;                      // "." entry
	rec = root + 34; rec[0] = 46; rec[2] = 20; rec[10] = 40; rec[32] = 12; memcpy(rec + 33, "SYSTEM.CNF;1", 12);
	memcpy(iso + 20 * SECTOR, "BOOT2 = cdrom0:\\SLUS_209.46;1\r\nVER = 1.00\r\n", 40);
	rec = root + 80; rec[0] = 38; rec[2] = 17; rec[11] = SECTOR >> 8; rec[25] = 2; rec[32] = 4; memcpy(rec + 33, "DATA", 4); // dir
	rec = iso + 17 * SECTOR; rec[0] = 44; rec[2] = 19; rec[10] = 5; rec[32] = 10; memcpy(rec + 33, "MAIN.ELF;1", 10);
	FILE *f = tmpfile();
	fwrite(iso, 1, sizeof(iso), f);
	char s[16], d[16], t[64];
	unsigned lba, size;
	assert(iso_find(read_file, f, "\\data\\main.elf", &lba, &size) && lba == 19 && size == 5);
	assert(iso_find(read_file, f, "SYSTEM.CNF", &lba, &size) && lba == 20 && size == 40);
	assert(!iso_find(read_file, f, "DATA\\MAIN.EL", &lba, &size) && !iso_find(read_file, f, "MAIN.ELF", &lba, &size));
    /* Malformed directory-record length and root-record length must fail closed. */
    root[34] = 33; rewind(f); fwrite(iso, 1, sizeof(iso), f);
    assert(!iso_find(read_file, f, "SYSTEM.CNF", &lba, &size));
    root[34] = 46; pvd[156] = 0; rewind(f); fwrite(iso, 1, sizeof(iso), f);
    assert(!iso_find(read_file, f, "SYSTEM.CNF", &lba, &size));
    pvd[156] = 34; rewind(f); fwrite(iso, 1, sizeof(iso), f);
	assert(iso_serial(f, s) && !strcmp(s, "SLUS_209.46"));
	memcpy(iso + 20 * SECTOR, "BOOT = cdrom:\\SLPS_123.45;1\r\n\0\0\0\0\0\0\0\0\0\0\0", 40); // PS1: no BOOT2
	rewind(f); fwrite(iso, 1, sizeof(iso), f);
	assert(!iso_serial(f, s));
	assert((iso_serial)(read_file, f, 1, s) && !strcmp(s, "SLPS_123.45")); // PS1 mode (bypasses the macro)
	char raw[64];
	assert(cnf_boot("VER = 1\r\nBOOT2 = cdrom0:\\DATA\\MAIN.ELF;1\r\n", 0, s, raw, sizeof(raw)));
	assert(!strcmp(s, "MAIN.ELF") && !strcmp(raw, "cdrom0:\\DATA\\MAIN.ELF;1"));
	assert(!cnf_boot("BOOT2 = cdrom0:\\SLUS_209.46;1\n", 1, s, NULL, 0)); // PS2 file: no PS1 line
	assert(!cnf_boot("BOOT = cdrom:\\SCUS_941.63;1\n", 0, s, NULL, 0)); // PS1 file: no PS2 line
	assert(cnf_boot("BOOT=cdrom:SCUS_941.63;1\n", 1, s, NULL, 0) && !strcmp(s, "SCUS_941.63"));
	pvd[1] = 'X'; rewind(f); fwrite(iso, 1, sizeof(iso), f);
	assert(!iso_serial(f, s));                                  // not an ISO
	fclose(f);
	serial_dash("SLUS_209.46", d);
	assert(!strcmp(d, "SLUS-20946"));
	iso_title("SLUS_209.46.God of War.iso", t, sizeof(t));
	assert(!strcmp(t, "God of War"));
	iso_title("Crash Bandicoot.VCD", t, sizeof(t));
	assert(!strcmp(t, "Crash Bandicoot"));
	iso_title("Okami (USA).ISO", t, sizeof(t));
	assert(!strcmp(t, "Okami (USA)"));
	iso_title("SLUS_209.46.God of War.zso", t, sizeof(t));
	assert(!strcmp(t, "God of War"));
	assert(name_serial("SLUS_209.46.God of War.iso", s) && !strcmp(s, "SLUS_209.46"));
	assert(!name_serial("God of War.iso", s) && !name_serial("SLUS 209.46.X.iso", s));
    char bad_cat[] = "DVD/../escape.iso\\t1\\tSLUS_213.76\\t8ee64a784fa9c9ac0a828be9a24f3679\\tBad\\n"
                        "DVD/Bad.iso\\t-1\\tSLUS_213.76\\t8ee64a784fa9c9ac0a828be9a24f3679\\tBad\\n"
                        "DVD/Bad.iso\\t1x\\tSLUS_213.76\\t8ee64a784fa9c9ac0a828be9a24f3679\\tBad\\n";
    char *bp = bad_cat;
    cat_ent bad_entry;
    assert(!catalog_next(&bp, &bad_entry));
	char cat[] = "DVD/Black.iso\t3919609856\tSLUS_213.76\t8ee64a784fa9c9ac0a828be9a24f3679\tBlack\r\n"
	             "broken line\n\n"
	             "CD/SLUS_123.45.Café Ñu.iso\t700\t-\t-\tCafé Ñu\n"
	             "DVD/x.iso\t1\tS\tH\t\n";                               // no title: skipped
	char *cp = cat;
	cat_ent ce;
	assert(catalog_next(&cp, &ce) && !strcmp(ce.path, "DVD/Black.iso") && ce.size == 3919609856ULL &&
	       !strcmp(ce.serial, "SLUS_213.76") && !strcmp(ce.hash, "8ee64a784fa9c9ac0a828be9a24f3679") && !strcmp(ce.title, "Black"));
	assert(catalog_next(&cp, &ce) && !strcmp(ce.title, "Café Ñu") && !strcmp(ce.serial, "-") && ce.size == 700);
	assert(!catalog_next(&cp, &ce) && !catalog_next(&cp, &ce));
	puts("iso selftest ok");
	return 0;
}
#endif
