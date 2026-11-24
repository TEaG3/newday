#!/usr/bin/env python3
"""Patch the pinned upstream OPL SMB backend for TEa.G3 single-game auto-launch."""
from pathlib import Path
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: patch_opl_eth.py /path/to/src/opl.c /path/to/src/ethsupport.c")

opl_path, eth_path = map(Path, sys.argv[1:])
opl = opl_path.read_text(encoding="utf-8")
eth = eth_path.read_text(encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one source anchor, found {count}")
    return text.replace(old, new, 1)


eth = replace_once(
    eth,
    "static int ethGameCount = 0;\n",
    "static int ethGameCount = 0;\nstatic int ethDirectLaunch = 0; // suppress GUI-only prompts during TEa.G3 auto-launch\n",
    "direct-launch state",
)

eth = replace_once(
    eth,
    "void ethDisplayErrorStatus(void)\n{\n    switch (gNetworkStartup) {",
    "void ethDisplayErrorStatus(void)\n{\n    if (ethDirectLaunch) {\n        LOG(\"TEa.G3 OPL SMB startup status: %d\\n\", gNetworkStartup);\n        return;\n    }\n    switch (gNetworkStartup) {",
    "non-interactive network errors",
)

eth = replace_once(
    eth,
    """                char error[256];
                snprintf(error, sizeof(error), _l(_STR_ERR_VMC_CONTINUE), vmc_name, (vmc_id + 1));
                if (!guiMsgBox(error, 1, NULL))
                    return;""",
    """                if (!ethDirectLaunch) {
                    char error[256];
                    snprintf(error, sizeof(error), _l(_STR_ERR_VMC_CONTINUE), vmc_name, (vmc_id + 1));
                    if (!guiMsgBox(error, 1, NULL))
                        return;
                } else {
                    LOG("TEa.G3 OPL SMB: unavailable VMC %s; continuing without it\\n", vmc_name);
                }""",
    "VMC prompt suppression",
)

eth = replace_once(
    eth,
    """    if ((result = sbLoadCheats(ethPrefix, game->startup, configSet)) < 0) {
        switch (result) {
            case -ENOENT:
                guiWarning(_l(_STR_NO_CHEATS_FOUND), 10);
                break;
            default:
                guiWarning(_l(_STR_ERR_CHEATS_LOAD_FAILED), 10);
        }
    }""",
    """    if ((result = sbLoadCheats(ethPrefix, game->startup, configSet)) < 0) {
        if (ethDirectLaunch)
            LOG("TEa.G3 OPL SMB: cheats unavailable (%d); continuing\\n", result);
        else switch (result) {
            case -ENOENT:
                guiWarning(_l(_STR_NO_CHEATS_FOUND), 10);
                break;
            default:
                guiWarning(_l(_STR_ERR_CHEATS_LOAD_FAILED), 10);
        }
    }""",
    "cheat prompt suppression",
)

helper = r'''
static int ethParseIPv4(const char *text, int out[4])
{
    int a, b, c, d;
    char tail;
    if (!text || sscanf(text, "%d.%d.%d.%d%c", &a, &b, &c, &d, &tail) != 4 ||
        a < 0 || a > 255 || b < 0 || b > 255 ||
        c < 0 || c > 255 || d < 0 || d > 255)
        return 0;
    out[0] = a, out[1] = b, out[2] = c, out[3] = d;
    return 1;
}

/* argv: [0] dummy, [1] DVD/file.iso or CD/file.iso, [2] startup,
   [3] CD|DVD, [4] eth, [5] PS2 IP, [6] mask, [7] gateway,
   [8] DNS, [9] SMB server, [10] share, [11] user, [12] password. */
int ethAutoLaunchGame(char *argv[])
{
    char image[ISO_GAME_FNAME_MAX + 1];
    int nameLen = 0;
    int format;
    const char *filename;

    if (!argv || !argv[1] || !argv[2] || !argv[3] || !argv[4] ||
        !argv[5] || !argv[6] || !argv[7] || !argv[8] ||
        !argv[9] || !argv[10] || !argv[11] || !argv[12])
        return -1;

    if (!ethParseIPv4(argv[5], ps2_ip) || !ethParseIPv4(argv[6], ps2_netmask) ||
        !ethParseIPv4(argv[9], pc_ip) || strlen(argv[10]) == 0 || strlen(argv[10]) >= sizeof(gPCShareName) ||
        strlen(argv[11]) >= sizeof(gPCUserName) || strlen(argv[12]) >= sizeof(gPCPassword))
        return -1;
    if (argv[7][0]) {
        if (!ethParseIPv4(argv[7], ps2_gateway)) return -1;
    } else {
        memset(ps2_gateway, 0, sizeof(ps2_gateway));
    }
    if (argv[8][0]) {
        if (!ethParseIPv4(argv[8], ps2_dns)) return -1;
    } else {
        memset(ps2_dns, 0, sizeof(ps2_dns));
    }

    if (!strcmp(argv[3], "CD") && !strncmp(argv[1], "CD/", 3))
        filename = argv[1] + 3;
    else if (!strcmp(argv[3], "DVD") && !strncmp(argv[1], "DVD/", 4))
        filename = argv[1] + 4;
    else
        return -1;

    if (strlen(filename) == 0 || strlen(filename) >= sizeof(image) ||
        strlen(argv[2]) != GAME_STARTUP_MAX - 1)
        return -1;
    strcpy(image, filename);
    format = isValidIsoName(image, &nameLen);
    if (!format || nameLen <= 0 || nameLen > ISO_GAME_NAME_MAX)
        return -1;

    ps2_ip_use_dhcp = 0;
    gETHOpMode = ETH_OP_MODE_AUTO;
    gPCShareAddressIsNetBIOS = 0;
    gPCShareNBAddress[0] = '\0';
    gPCPort = 445;
    gPCShareName[0] = gPCUserName[0] = gPCPassword[0] = '\0';
    snprintf(gPCShareName, sizeof(gPCShareName), "%s", argv[10]);
    snprintf(gPCUserName, sizeof(gPCUserName), "%s", argv[11]);
    snprintf(gPCPassword, sizeof(gPCPassword), "%s", argv[12]);
    gETHPrefix[0] = '\0';
    ethBase = "smb0:";
    ethDirectLaunch = 1;
    gNetworkStartup = ERROR_ETH_NOT_STARTED;

    ethInit(&ethGameList);
    for (int t = 0; t < 800 && gNetworkStartup == ERROR_ETH_NOT_STARTED; t++)
        usleep(50000);
    if (gNetworkStartup != 0) {
        LOG("TEa.G3 OPL SMB: connection setup failed (%d)\\n", gNetworkStartup);
        return -1;
    }

    ethGames = calloc(1, sizeof(base_game_info_t));
    if (!ethGames)
        return -1;
    memset(ethGames, 0, sizeof(base_game_info_t));
    if (format == GAME_FORMAT_OLD_ISO) {
        strncpy(ethGames[0].name, &image[GAME_STARTUP_MAX], nameLen);
        ethGames[0].name[nameLen] = '\0';
        strncpy(ethGames[0].extension, &image[GAME_STARTUP_MAX + nameLen], sizeof(ethGames[0].extension) - 1);
    } else {
        strncpy(ethGames[0].name, image, nameLen);
        ethGames[0].name[nameLen] = '\0';
        strncpy(ethGames[0].extension, &image[nameLen], sizeof(ethGames[0].extension) - 1);
    }
    snprintf(ethGames[0].startup, sizeof(ethGames[0].startup), "%s", argv[2]);
    ethGames[0].format = format;
    ethGames[0].media = !strcmp(argv[3], "CD") ? SCECdPS2CD : SCECdPS2DVD;
    ethGames[0].parts = 1;
    ethGames[0].sizeMB = 0;
    ethGameCount = 1;

    config_set_t *configSet = sbPopulateConfig(&ethGames[0], ethPrefix, "\\");
    if (!configSet)
        return -1;

    ethLaunchGame(&ethGameList, 0, configSet);
    /* ethLaunchGame deinitializes OPL before starting CDVDMAN; never deinit twice. */
    return 0;
}

'''
eth = replace_once(
    eth,
    "static int ethReadNetConfig(void)\n{",
    helper + "static int ethReadNetConfig(void)\n{",
    "SMB direct-launch adapter insertion",
)

header_path = eth_path.parent.parent / "include" / "ethsupport.h"
header = header_path.read_text(encoding="utf-8")
header = replace_once(
    header,
    "item_list_t *ethGetObject(int initOnly);\n",
    "item_list_t *ethGetObject(int initOnly);\nint ethAutoLaunchGame(char *argv[]);\n",
    "SMB adapter declaration",
)

old_main = """        if (!strcmp(argv[4], "ul") && argc >= 6) {
            autoLaunchBDMGame(argv);
            return 0;
        }
"""
new_main = """        if (!strcmp(argv[4], "ul") && argc >= 6) {
            autoLaunchBDMGame(argv);
            return 0;
        }
        if (!strcmp(argv[4], "eth") && argc >= 13) {
            init();
            list_support[ETH_MODE].support = ethGetObject(0);
            if (ethAutoLaunchGame(argv) == 0)
                return 0;
            deinit(NO_EXCEPTION, ETH_MODE);
            return 1;
        }
"""
opl = replace_once(opl, old_main, new_main, "OPL SMB auto-launch dispatch")

eth_path.write_text(eth, encoding="utf-8")
header_path.write_text(header, encoding="utf-8")
opl_path.write_text(opl, encoding="utf-8")
print("OPL SMB/ETH direct-launch patch applied")
