#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
src = root / "iop" / "smb" / "src" / "smb_ioman.c"
text = src.read_text()

if "smb_dev_dopen" not in text:
    anchor = """static int sema;

static int smb_nop(void)
"""
    insert = """static int sema;

#define MAX_DIRS 4
static struct {
        uint8_t *handle;
} dirs[MAX_DIRS];

/* usmb2 FileDirectoryInformation layout returned by usmb2_readdir:
 * attributes at byte 56, EOF size at byte 40, filename length at byte 60,
 * followed by a nul-terminated ASCII filename at byte 64.
 */
static uint32_t smb_le32(const uint8_t *p)
{
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
               ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int smb_nop(void)
"""
    if anchor not in text:
        raise SystemExit("smb_ioman.c: semaphore insertion point not found")
    text = text.replace(anchor, insert, 1)

    anchor = """static s64 smb_dev_lseek64(iomanX_iop_file_t *f, s64 offset, int whence)
"""
    insert = """static int smb_dev_dopen(iomanX_iop_file_t *f, const char *name)
{
        int i, rc = -EMFILE;

        WaitSema(sema);
        if (smb == NULL && smb_connect()) {
                rc = -EIO;
                goto out;
        }
        for (i = 0; i < MAX_DIRS; i++) {
                if (dirs[i].handle == NULL)
                        break;
        }
        if (i == MAX_DIRS)
                goto out;

        dirs[i].handle = usmb2_opendir(smb, name);
        if (dirs[i].handle == NULL) {
                rc = -ENOENT;
                goto out;
        }
        f->privdata = &dirs[i];
        rc = 0;
out:
        SignalSema(sema);
        return rc;
}

static int smb_dev_dclose(iomanX_iop_file_t *f)
{
        typeof(dirs[0]) *dir = f->privdata;

        if (dir == NULL || dir->handle == NULL)
                return -EBADF;
        WaitSema(sema);
        usmb2_closedir(smb, dir->handle);
        dir->handle = NULL;
        f->privdata = NULL;
        SignalSema(sema);
        return 0;
}

static int smb_dev_dread(iomanX_iop_file_t *f, iox_dirent_t *out)
{
        typeof(dirs[0]) *dir = f->privdata;
        uint8_t *entry;
        uint32_t attrs, size;
        const char *name;
        int rc = 0;

        if (dir == NULL || dir->handle == NULL || out == NULL)
                return -EBADF;

        WaitSema(sema);
        entry = usmb2_readdir(smb, dir->handle);
        if (entry == NULL)
                goto done;

        attrs = smb_le32(entry + 56);
        size = smb_le32(entry + 40);
        name = (const char *)(entry + 64);
        memset(out, 0, sizeof(*out));
        strncpy(out->name, name, sizeof(out->name) - 1);
        out->stat.mode = FIO_S_IROTH | FIO_S_IXOTH;
        if (attrs & 0x10)
                out->stat.mode |= FIO_S_IFDIR;
        else
                out->stat.mode |= FIO_S_IFREG;
        out->stat.size = size;
        rc = 1;
done:
        SignalSema(sema);
        return rc;
}

static s64 smb_dev_lseek64(iomanX_iop_file_t *f, s64 offset, int whence)
"""
    if anchor not in text:
        raise SystemExit("smb_ioman.c: lseek insertion point not found")
    text = text.replace(anchor, insert, 1)

    text = text.replace(
        """        (void *)smb_eio, /* dopen */
        (void *)smb_eio, /* dclose */
        (void *)smb_eio, /* dread */
""",
        """        smb_dev_dopen,
        smb_dev_dclose,
        smb_dev_dread,
""",
        1
    )

    src.write_text(text)

makefile = root / "iop" / "smb" / "Makefile"
mk = makefile.read_text()
flag = "-DUSMB2_FEATURE_TINY_TCP -DUSMB2_FEATURE_REASSEMBLY -DUSMB2_FEATURE_NTLM"
if "-DUSMB2_FEATURE_OPENDIR" not in mk:
    if flag not in mk:
        raise SystemExit("iop/smb/Makefile: feature flag anchor not found")
    mk = mk.replace(flag, flag + " -DUSMB2_FEATURE_OPENDIR", 1)
    makefile.write_text(mk)

print("SMB directory enumeration patch applied")
