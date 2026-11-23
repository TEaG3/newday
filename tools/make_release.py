#!/usr/bin/env python3
"""Build the release zip: dist/TEa.G3-PS2.zip (run after build.sh APP=launcher, tools/build_neutrino.sh and
build.sh nuld_hash). Usage: python tools/make_release.py v1.0 <neutrino dev release dir> <udpfs_server.py>

Layout, to be copied to the root of the USB drive (server/ goes to the game server instead):
  TEa.G3.ELF  DVD/  CD/  (OPL and Neutrino are embedded in TEa.G3.ELF)
  server/ (nuld_catalog.py, nuld_hash, udpfs_server.py, systemd units)  tools/covers.py  QUICKSTART.txt  LICENSE
"""
import hashlib, os, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ver, ndev, udpfs = sys.argv[1], sys.argv[2], sys.argv[3]
name = "TEa.G3-PS2"
out = os.path.join(ROOT, "dist", name + ".zip")

QUICKSTART = f"""TEa.G3 {ver} - quick start
============================

1. Copy TEa.G3.ELF, OPL.ELF, neutrino/, DVD/ and CD/ to the root of a FAT32 or exFAT USB drive.\n   TEa.G3 starts the selected engine automatically. OPL.ELF is a runtime component; do not launch it manually.
2. Put your PS2 ISOs in DVD/ (CD games in CD/). PS1 games: POPStarter VCDs in POPS/. Homebrew: APPS/.
3. On the PS2, start mass0:/TEa.G3.ELF (uLaunchELF, or FreeMcBoot's autoboot).
4. Pick a game and press X. The first boot writes mass0:/tea_g3/config.ini with every setting, commented.

Buttons: left/right (or up/down) move, L1/R1 filter, square changes the view, triangle per-game options,
circle achievements, X play, SELECT data overlay.
In a game: L1+L2+R1+R2+START+SELECT opens the menu (restart, power off).

Optional
- English / Português: in config.ini set  [ui] idioma = en  or  idioma = pt  (Spanish is the default).
- Covers: plug in the network cable; the missing ones download at boot.
  Your own: tools/covers.py turns PNG/JPG files named by serial (SLUS-21376.png) into the .c16 pair for
  mass0:/covers/ (python3 covers.py IN_DIR OUT_DIR; needs numpy and Pillow).
  No DNS on your router? Set  [red] dns = 1.1.1.1  in config.ini.
- Back to TEa.G3 from a game: TEa.G3 installs BOOT/TEa.G3.ELF on your memory card; set it as FMCB's autoboot.
- Coming from NULD or ORBIT (v1.1 or older): the first boot moves mass0:/nuld/ or mass0:/orbit/ to mass0:/tea_g3/, keeps BOOT/ORBIT.ELF
  up to date (an FMCB autoboot pointing at it keeps working) and reuses VMC/ORBIT.bin or VMC/NULD.bin when present; new installs use VMC/TEa.G3.bin.
- Achievements: run the xeRAbora client v0.1.0-alpha.9, signed in, on your network
  (github.com/hacan359/xerabora/releases/tag/v0.1.0-alpha.9: the version TEa.G3 is tested with).
- Games from a home server: see server/README.txt (wired network needed).

Website and setup video: https://danielnuld.github.io/nuld/
Source: https://github.com/danielnuld/nuld (GPL-3.0; the Neutrino fork is AFL-3.0)
"""

SERVER = """TEa.G3 game server (Linux)
=========================

Serves your ISOs to the PS2 over the network, with udpfs (Neutrino's) and TEa.G3's catalog.

1. Put the ISOs in <root>/DVD (and <root>/CD), e.g. /srv/data/ps2/DVD.
2. Copy this folder to the server, e.g. ~/nuld, and install both services (user services):
     mkdir -p ~/.config/systemd/user && cp *.service ~/.config/systemd/user/
     edit the --root in both if your folder is not /srv/data/ps2
     systemctl --user daemon-reload && systemctl --user enable --now udpfs nuld-catalog
     loginctl enable-linger $USER      (keep them running without a login)
3. Allow UDP 62966 (udpfs) and TCP 18290 (catalog) in the firewall.
4. On the PS2, config.ini:  [juegos] origen = usb, udpfs   and   servidor = <server IP>
5. Use a cable: udpfs does not survive WiFi packet loss.

nuld_hash is a static x86_64 Linux binary (the launcher's own ISO and memory-card code). nuld_catalog.py needs
only Python 3. curl http://<server>:18290/catalog lists what the PS2 will see.
"""

UNITS = {
    "nuld-catalog.service": """[Unit]
Description=TEa.G3 game catalog for the PS2 launcher
After=network-online.target

[Service]
Environment=PYTHONUNBUFFERED=1
ExecStart=/usr/bin/python3 %h/nuld/nuld_catalog.py --root /srv/data/ps2 --port 18290
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
""",
    "udpfs.service": """[Unit]
Description=UDPFS server for the PS2 (Neutrino)
After=network-online.target

[Service]
Environment=PYTHONUNBUFFERED=1
ExecStart=/usr/bin/python3 %h/nuld/udpfs_server.py --root-dir /srv/data/ps2
Restart=on-failure
RestartSec=5

[Install]
WantedBy=default.target
""",
}

os.makedirs(os.path.dirname(out), exist_ok=True)
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    def add(src, arc, mode=0o644):
        info = zipfile.ZipInfo.from_file(src, f"{name}/{arc}")
        info.external_attr = (mode | 0o100000) << 16
        with open(src, "rb") as f:
            z.writestr(info, f.read(), zipfile.ZIP_DEFLATED)

    def text(arc, s, mode=0o644):
        info = zipfile.ZipInfo(f"{name}/{arc}", (2026, 10, 4, 0, 0, 0))
        info.external_attr = (mode | 0o100000) << 16
        z.writestr(info, s.replace("\n", "\r\n") if arc.endswith(".txt") else s, zipfile.ZIP_DEFLATED)

    add(os.path.join(ROOT, "TEa.G3.ELF"), "TEa.G3.ELF")
    # The runtime payload is linked into TEa.G3.ELF and extracted to tea_g3/runtime on first boot.
    manifest = os.path.join(ROOT, "runtime-manifest.txt")
    if not os.path.isfile(manifest) or os.path.getsize(manifest) == 0:
        raise SystemExit("runtime-manifest.txt is missing: build the launcher with embedded OPL and Neutrino first")
    with open(manifest, encoding="utf-8") as f:
        embedded = f.read()
    for required in ("OPL.ELF", "neutrino/neutrino.elf", "neutrino/modules/ee_core.elf",
                     "neutrino/modules/raagent.irx", "neutrino/modules/smb_fhi.irx"):
        if required not in embedded:
            raise SystemExit("embedded runtime is incomplete: " + required)
    add(manifest, "runtime-manifest.txt")
    text("DVD/put your PS2 DVD ISOs here.txt", "")
    text("CD/put your PS2 CD ISOs here.txt", "")
    add(os.path.join(ROOT, "tools", "covers.py"), "tools/covers.py", 0o755)
    add(os.path.join(ROOT, "tools", "nuld_catalog.py"), "server/nuld_catalog.py", 0o755)
    add(os.path.join(ROOT, "nuld_hash"), "server/nuld_hash", 0o755)
    add(udpfs, "server/udpfs_server.py", 0o755)
    for n, s in UNITS.items():
        text("server/" + n, s)
    text("server/README.txt", SERVER)
    text("QUICKSTART.txt", QUICKSTART)
    add(os.path.join(ROOT, "LICENSE"), "LICENSE")

h = hashlib.sha256(open(out, "rb").read()).hexdigest().upper()
print(out, os.path.getsize(out), "bytes")
print("TEa.G3.ELF SHA-256", hashlib.sha256(open(os.path.join(ROOT, "TEa.G3.ELF"), "rb").read()).hexdigest().upper())
print("zip SHA-256", h)

# self-update assets (#15): update.txt + update.sig + TEa.G3's files, uploaded next to the zip
import subprocess
subprocess.run([sys.executable, os.path.join(ROOT, "tools", "make_update.py"), ver], check=True)
