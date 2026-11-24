#!/usr/bin/env python3
"""Patch the pinned OPL auto-launch adapter for UL and source-specific BDM routes."""
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_opl_ul.py /path/to/Open-PS2-Loader/src/opl.c")

path = Path(sys.argv[1])
source = path.read_text(encoding="utf-8")

old = r'''    int nameLen;
    int format = isValidIsoName(argv[1], &nameLen);
    if (format == GAME_FORMAT_OLD_ISO) {
        strncpy(gAutoLaunchBDMGame->name, &argv[1][GAME_STARTUP_MAX], nameLen);
        gAutoLaunchBDMGame->name[nameLen] = '\0';
        strncpy(gAutoLaunchBDMGame->extension, &argv[1][GAME_STARTUP_MAX + nameLen], sizeof(gAutoLaunchBDMGame->extension));
        gAutoLaunchBDMGame->extension[sizeof(gAutoLaunchBDMGame->extension) - 1] = '\0';
    } else {
        strncpy(gAutoLaunchBDMGame->name, argv[1], nameLen);
        gAutoLaunchBDMGame->name[nameLen] = '\0';
        strncpy(gAutoLaunchBDMGame->extension, &argv[1][nameLen], sizeof(gAutoLaunchBDMGame->extension));
        gAutoLaunchBDMGame->extension[sizeof(gAutoLaunchBDMGame->extension) - 1] = '\0';
    }

    snprintf(gAutoLaunchBDMGame->startup, sizeof(gAutoLaunchBDMGame->startup), argv[2]);
'''
new = r'''    int isUL = !strcmp(argv[4], "ul");
    int isBDM = !strcmp(argv[4], "bdm");
    int isUSB = isUL || (!strcmp(argv[4], "usb")) || (isBDM && !strcmp(argv[5], "usb"));
    const char *wantedDriver = isUL || !strcmp(argv[4], "usb") ? "usb" : isBDM ? argv[5] : "";
    int wantedUnit = 0;
    if (isBDM) {
        char *end;
        unsigned long unit = strtoul(argv[6], &end, 10);
        if (*end || unit > 4 || (strcmp(wantedDriver, "usb") && strcmp(wantedDriver, "sd") &&
            strcmp(wantedDriver, "sdc") && strcmp(wantedDriver, "ata")))
            return;
        wantedUnit = (int)unit;
    }

    int format;
    if (isUL) {
        char *end;
        unsigned long parts = strtoul(argv[5], &end, 10);
        size_t nameLen = strlen(argv[1]);
        if (nameLen == 0 || nameLen > UL_GAME_NAME_MAX || strlen(argv[2]) != GAME_STARTUP_MAX - 1 ||
            parts == 0 || parts > 255 || *end != '\0')
            return;
        memcpy(gAutoLaunchBDMGame->name, argv[1], nameLen);
        gAutoLaunchBDMGame->name[nameLen] = '\0';
        format = GAME_FORMAT_USBLD;
        gAutoLaunchBDMGame->parts = (u8)parts;
    } else {
        int nameLen;
        format = isValidIsoName(argv[1], &nameLen);
        if (format == 0 || nameLen <= 0 || nameLen > ISO_GAME_NAME_MAX)
            return;
        if (format == GAME_FORMAT_OLD_ISO) {
            strncpy(gAutoLaunchBDMGame->name, &argv[1][GAME_STARTUP_MAX], nameLen);
            gAutoLaunchBDMGame->name[nameLen] = '\0';
            strncpy(gAutoLaunchBDMGame->extension, &argv[1][GAME_STARTUP_MAX + nameLen], sizeof(gAutoLaunchBDMGame->extension));
            gAutoLaunchBDMGame->extension[sizeof(gAutoLaunchBDMGame->extension) - 1] = '\0';
        } else {
            strncpy(gAutoLaunchBDMGame->name, argv[1], nameLen);
            gAutoLaunchBDMGame->name[nameLen] = '\0';
            strncpy(gAutoLaunchBDMGame->extension, &argv[1][nameLen], sizeof(gAutoLaunchBDMGame->extension));
            gAutoLaunchBDMGame->extension[sizeof(gAutoLaunchBDMGame->extension) - 1] = '\0';
        }
        gAutoLaunchBDMGame->parts = 1;
    }

    snprintf(gAutoLaunchBDMGame->startup, sizeof(gAutoLaunchBDMGame->startup), "%s", argv[2]);
'''
if source.count(old) != 1:
    raise SystemExit(f"expected one original autoLaunchBDMGame argument block, found {source.count(old)}")
source = source.replace(old, new, 1)

old = '    gAutoLaunchBDMGame->parts = 1; // ul not supported.\n'
if source.count(old) != 1:
    raise SystemExit(f"expected one old UL-disabled parts assignment, found {source.count(old)}")
source = source.replace(old, '', 1)

old = r'''            if (!strcmp(gAutoLaunchDeviceData->bdmDriver, "ata") && strlen(gAutoLaunchDeviceData->bdmDriver) == 3) {
                bdmResolveLBA_UDMA(gAutoLaunchDeviceData);
                snprintf(apaDevicePrefix, sizeof(apaDevicePrefix), "mass%d:", i);
                fileXioDclose(dir);
                break; // Exit the loop if "ata" device is found
            }
'''
new = r'''            int matches = !strcmp(gAutoLaunchDeviceData->bdmDriver, wantedDriver) &&
                          (!isBDM || gAutoLaunchDeviceData->massDeviceIndex == wantedUnit);
            if (matches && !strcmp(gAutoLaunchDeviceData->bdmDriver, "ata"))
                bdmResolveLBA_UDMA(gAutoLaunchDeviceData);
            if (matches) {
                snprintf(apaDevicePrefix, sizeof(apaDevicePrefix), "mass%d:", i);
                fileXioDclose(dir);
                break;
            }
'''
if source.count(old) != 1:
    raise SystemExit(f"expected one BDM device-selection block, found {source.count(old)}")
source = source.replace(old, new, 1)

old = r'''    if (gBDMPrefix[0] != '\0') {'''
new = r'''    if (isBDM || isUL || !strcmp(argv[4], "usb"))
        gBDMPrefix[0] = '\0'; // TEa.G3 passes paths relative to the selected device root.
    if (!isUL && gBDMPrefix[0] != '\0') {'''
if source.count(old) != 1:
    raise SystemExit(f"expected one BDM prefix branch, found {source.count(old)}")
source = source.replace(old, new, 1)

old = r'''        if (!strcmp(argv[4], "bdm"))
            autoLaunchBDMGame(argv);
'''
new = r'''        if (!strcmp(argv[4], "bdm") && argc >= 7) {
            autoLaunchBDMGame(argv);
            return 0;
        }
        if (!strcmp(argv[4], "usb")) {
            autoLaunchBDMGame(argv);
            return 0;
        }
        /* argv[0] boot path, argv[1] raw UL name, argv[2] startup ID,
           argv[3] "CD"/"DVD", argv[4] "ul", argv[5] split-part count. */
        if (!strcmp(argv[4], "ul") && argc >= 6) {
            autoLaunchBDMGame(argv);
            return 0;
        }
'''
if source.count(old) != 1:
    raise SystemExit(f"expected one BDM main dispatch, found {source.count(old)}")
source = source.replace(old, new, 1)

path.write_text(source, encoding="utf-8")
print("OPL TEa.G3 direct-launch routes patched: UL + USB/BDM ISO/ZSO")
