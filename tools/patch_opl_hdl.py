#!/usr/bin/env python3
"""Patch upstream OPL for safe TEa.G3 APA/HD Loader mini-launch."""
from pathlib import Path
import sys

if len(sys.argv) != 4:
    raise SystemExit("usage: patch_opl_hdl.py src/opl.c src/hddsupport.c include/hddsupport.h")

opl_path, hdd_path, header_path = map(Path, sys.argv[1:])
opl = opl_path.read_text(encoding="utf-8")
hdd = hdd_path.read_text(encoding="utf-8")
header = header_path.read_text(encoding="utf-8")


def once(source, old, new, label):
    count = source.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one anchor, found {count}")
    return source.replace(old, new, 1)


# The mini-launch flag makes HDD discovery strictly read-only until the
# existing OPL partition has been found and mounted. It also suppresses the
# upstream fallback that creates +OPL when the configured partition is absent.
header = once(
    header,
    "void hddLoadSupportModules(void);\n",
    "void hddLoadSupportModules(void);\n"
    "extern int gTEaG3HDDMiniMode;\n"
    "extern int gTEaG3HDDMiniReady;\n",
    "HDD mini-launch state declarations",
)
opl = once(
    opl,
    "hdl_game_info_t *gAutoLaunchGame;\n",
    "hdl_game_info_t *gAutoLaunchGame;\n"
    "int gTEaG3HDDMiniMode = 0;\n"
    "int gTEaG3HDDMiniReady = 0;\n",
    "HDD mini-launch state",
)

hdd = once(
    hdd,
    '''    char name[64];
    int fd, ret = 0;''',
    '''    char name[64] = {0};
    int fd, ret = 0;''',
    "initialize partition config buffer",
)

# Read the already existing __common/OPL/conf_hdd.cfg without creating or
# rewriting anything when this is a TEa.G3 mini-launch.
hdd = once(
    hdd,
    "extern u8 IOBuffer[2048];\n",
    "extern u8 IOBuffer[2048];\n"
    "extern int gTEaG3HDDMiniMode;\n"
    "extern int gTEaG3HDDMiniReady;\n",
    "HDD mini-launch state imports",
)
hdd = once(
    hdd,
    'ret = fileXioMount("pfs0:", "hdd0:__common", FIO_MT_RDWR);',
    'ret = fileXioMount("pfs0:", "hdd0:__common", gTEaG3HDDMiniMode ? FIO_MT_RDONLY : FIO_MT_RDWR);',
    "read-only OPL config mount",
)
hdd = once(
    hdd,
    '''            configGetStrCopy(config, "hdd_partition", name, sizeof(name));
            snprintf(gOPLPart, sizeof(gOPLPart), "hdd0:%s", name);

            configFree(config);
            close(fd);

            return;''',
    '''            configGetStrCopy(config, "hdd_partition", name, sizeof(name));
            if (gTEaG3HDDMiniMode) {
                int valid = name[0] != '\\0' && strlen(name) <= APA_IDMAX;
                for (const unsigned char *p = (const unsigned char *)name; valid && *p; p++)
                    if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
                          (*p >= '0' && *p <= '9') || *p == '.' || *p == '_' ||
                          *p == '-' || *p == '+'))
                        valid = 0;
                if (!valid) {
                    gOPLPart[0] = '\\0';
                    configFree(config);
                    close(fd);
                    fileXioUmount("pfs0:");
                    return;
                }
            }
            snprintf(gOPLPart, sizeof(gOPLPart), "hdd0:%s", name);

            configFree(config);
            close(fd);

            return;''',
    "validate configured OPL partition",
)
hdd = once(
    hdd,
    '''        hddCheckOPLFolder(hddPrefix);

        fd = open("pfs0:OPL/conf_hdd.cfg", O_CREAT | O_TRUNC | O_WRONLY);''',
    '''        if (gTEaG3HDDMiniMode) {
            gOPLPart[0] = '\\0';
            fileXioUmount("pfs0:");
            return;
        }

        hddCheckOPLFolder(hddPrefix);

        fd = open("pfs0:OPL/conf_hdd.cfg", O_CREAT | O_TRUNC | O_WRONLY);''',
    "do not create missing OPL config",
)
hdd = once(
    hdd,
    '''    snprintf(gOPLPart, sizeof(gOPLPart), "hdd0:+OPL");

    return;''',
    '''    if (gTEaG3HDDMiniMode) {
        gOPLPart[0] = '\\0';
        fileXioUmount("pfs0:");
        return;
    }
    snprintf(gOPLPart, sizeof(gOPLPart), "hdd0:+OPL");

    return;''',
    "do not guess default OPL partition",
)

# Do not create a missing PFS partition or OPL folder during mini-launch.
hdd = once(
    hdd,
    '''        if (gOPLPart[0] == '\\0')
            hddFindOPLPartition();

        fileXioUmount(hddPrefix);

        ret = fileXioMount(hddPrefix, gOPLPart, FIO_MT_RDWR);
        if (ret == -ENOENT) {
            // Attempt to create the partition.
            if ((hddCreateOPLPartition(gOPLPart)) >= 0)
                fileXioMount(hddPrefix, gOPLPart, FIO_MT_RDWR);
        }

        if (gOPLPart[5] != '+') {
            hddCheckOPLFolder(hddPrefix);
            gHDDPrefix = "pfs0:OPL/";
        }''',
    '''        if (gOPLPart[0] == '\\0')
            hddFindOPLPartition();

        if (gTEaG3HDDMiniMode && gOPLPart[0] == '\\0') {
            gTEaG3HDDMiniReady = 0;
            fileXioUmount(hddPrefix);
            return;
        }

        fileXioUmount(hddPrefix);
        ret = fileXioMount(hddPrefix, gOPLPart, FIO_MT_RDWR);
        if (ret == -ENOENT && !gTEaG3HDDMiniMode) {
            // Normal OPL GUI mode retains its upstream partition creation behavior.
            if ((hddCreateOPLPartition(gOPLPart)) >= 0)
                ret = fileXioMount(hddPrefix, gOPLPart, FIO_MT_RDWR);
        }
        if (ret < 0) {
            gTEaG3HDDMiniReady = 0;
            if (gTEaG3HDDMiniMode)
                fileXioUmount(hddPrefix);
            return;
        }

        if (gOPLPart[5] != '+') {
            if (gTEaG3HDDMiniMode) {
                DIR *dir = opendir("pfs0:OPL");
                if (!dir) {
                    gTEaG3HDDMiniReady = 0;
                    fileXioUmount(hddPrefix);
                    return;
                }
                closedir(dir);
            } else {
                hddCheckOPLFolder(hddPrefix);
            }
            gHDDPrefix = "pfs0:OPL/";
        }
        if (gTEaG3HDDMiniMode)
            gTEaG3HDDMiniReady = 1;''',
    "safe mount existing OPL partition only",
)

# The TEa.G3 argument is size_kb, not a partition name. OPL resolves the
# configured partition itself through the guarded read-only config lookup.
opl = once(
    opl,
    '''static void autoLaunchHDDGame(char *argv[])
{
    char path[256];
    config_set_t *configSet;

    miniInit(HDD_MODE);

    gAutoLaunchGame = malloc(sizeof(hdl_game_info_t));
    memset(gAutoLaunchGame, 0, sizeof(hdl_game_info_t));

    snprintf(gAutoLaunchGame->startup, sizeof(gAutoLaunchGame->startup), argv[1]);
    gAutoLaunchGame->start_sector = strtoul(argv[2], NULL, 0);
    snprintf(gOPLPart, sizeof(gOPLPart), "hdd0:%s", argv[3]);

    snprintf(path, sizeof(path), "%sCFG/%s.cfg", gHDDPrefix, gAutoLaunchGame->startup);
    configSet = configAlloc(0, NULL, path);
    configRead(configSet);

    hddLaunchGame(NULL, -1, configSet);
}''',
    '''static void autoLaunchHDDGame(char *argv[])
{
    char path[256], *end_sector, *end_size, *end_disc_type;
    unsigned long sector, size_kb, disc_type;
    config_set_t *configSet;

    if (!argv[1] || strlen(argv[1]) != 11 || !argv[2] || !argv[3] ||
        !argv[4] || !argv[5] || strcmp(argv[5], "mini"))
        return;
    sector = strtoul(argv[2], &end_sector, 0);
    size_kb = strtoul(argv[3], &end_size, 0);
    disc_type = strtoul(argv[4], &end_disc_type, 0);
    if (*end_sector || *end_size || *end_disc_type || !sector || !size_kb || !disc_type ||
        sector > 0xffffffffUL || size_kb > 0xffffffffUL || disc_type > 0xffUL)
        return;

    gTEaG3HDDMiniMode = 1;
    gTEaG3HDDMiniReady = 0;
    gOPLPart[0] = '\\0';
    miniInit(HDD_MODE);
    if (argv[6] && argv[6][0])
        snprintf(gExitPath, sizeof(gExitPath), "%s", argv[6]);
    gTEaG3HDDMiniMode = 0;
    if (!gTEaG3HDDMiniReady) {
        LOG("TEa.G3 HDL mini-launch refused: existing OPL HDD partition/config not available; no partition was created.\\n");
        return;
    }

    gAutoLaunchGame = calloc(1, sizeof(hdl_game_info_t));
    if (!gAutoLaunchGame)
        return;
    snprintf(gAutoLaunchGame->startup, sizeof(gAutoLaunchGame->startup), "%s", argv[1]);
    gAutoLaunchGame->start_sector = (u32)sector;
    gAutoLaunchGame->total_size_in_kb = (u32)size_kb;
    gAutoLaunchGame->disctype = (u8)disc_type;

    snprintf(path, sizeof(path), "%sCFG/%s.cfg", gHDDPrefix, gAutoLaunchGame->startup);
    configSet = configAlloc(0, NULL, path);
    if (!configSet) {
        LOG("TEa.G3 HDL mini-launch refused: could not allocate game config.\\n");
        free(gAutoLaunchGame);
        gAutoLaunchGame = NULL;
        return;
    }
    configRead(configSet);
    hddLaunchGame(NULL, -1, configSet);
}''',
    "safe HDL mini-launch arguments",
)

opl = once(
    opl,
    '''        if (!strcmp(argv[4], "mini"))
            autoLaunchHDDGame(argv);
        /* argv[0] boot path''',
    '''        if (argc >= 6 && !strcmp(argv[5], "mini")) {
            autoLaunchHDDGame(argv);
            return 0;
        }
        /* argv[0] boot path''',
    "exit after TEa.G3 HDL mini-launch",
)

opl_path.write_text(opl, encoding="utf-8")
hdd_path.write_text(hdd, encoding="utf-8")
header_path.write_text(header, encoding="utf-8")
print("OPL TEa.G3 APA/HDL mini-launch patched: exact LBA + full size; existing config only; no auto-create")
