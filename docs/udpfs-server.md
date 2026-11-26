# TEa.G3 UDPFS home game server

This guide documents the two host-side processes used by the TEa.G3 UDPFS source. They are **not** packaged in a `server/` directory in this repository, and this repository does not ship ready-made systemd unit files.

## What runs on the Linux host

1. Neutrino's UDPFS server exposes the game folders over UDP port **62966** (`0xF5F6`).
2. TEa.G3's catalog helper exposes the game list, app files, artwork and virtual-memory-card endpoints over HTTP port **18290**.

Keep both processes running. The PS2 must use a wired network connection.

## 1. Prepare the game share

Create the expected folders and place games in the usual layout:

```sh
sudo mkdir -p /srv/data/ps2/{DVD,CD,APPS,ART,VMC}
```

- PS2 DVD ISOs: `/srv/data/ps2/DVD/`
- PS2 CD ISOs: `/srv/data/ps2/CD/`
- Homebrew apps: `/srv/data/ps2/APPS/`
- Artwork: `/srv/data/ps2/ART/`
- Virtual memory cards: `/srv/data/ps2/VMC/`

## 2. Start Neutrino's UDPFS server

TEa.G3 builds its Neutrino runtime from the pinned fork revision `229cf1dc7a8e0e58c2da390c1caa63e2b61795dd`. The matching host server is `pc/udpfs_server.py` from that same repository revision.

```sh
sudo apt-get update
sudo apt-get install -y git python3 python3-venv

git clone https://github.com/ps2max32/neutrino.git "$HOME/tea-g3-neutrino"
git -C "$HOME/tea-g3-neutrino" checkout 229cf1dc7a8e0e58c2da390c1caa63e2b61795dd

python3 -m venv "$HOME/.venvs/tea-g3-udpfs"
. "$HOME/.venvs/tea-g3-udpfs/bin/activate"
python -m pip install --upgrade pip lz4

python "$HOME/tea-g3-neutrino/pc/udpfs_server.py" \
  --root-dir /srv/data/ps2 \
  --read-only \
  --enable-compression
```

The server listens on UDP 62966 by default. `--enable-compression` enables host-side ZSO/LZ4, CSO/zlib and CHD support where the corresponding libraries are installed. For ZSO, the Python `lz4` package is required. This host capability does not override TEa.G3's own source/engine compatibility matrix; check the current game-format guide before claiming a particular PS2 launch route.

The server is started in the foreground by this command. If you want it managed by systemd, create and review your own unit file using the same command and a dedicated unprivileged account; no unit file is shipped here.

## 3. Build and start the TEa.G3 catalog helper

On the Linux host, install a C compiler and OpenSSL development headers, then clone this repository:

```sh
sudo apt-get install -y build-essential libssl-dev

git clone https://github.com/TEaG3/newday.git "$HOME/TEa.G3"
cd "$HOME/TEa.G3"

cc -std=gnu99 -O2 -Wall -Wno-deprecated-declarations \
  -DSELFTEST -DSELFTEST_LIB -DNULD_HASH \
  tools/nuld_hash.c achievements.c iso.c vmc.c \
  -lcrypto -lpthread -o "$PWD/tea-g3-catalog-hash"

mkdir -p "$HOME/.cache"
python3 tools/nuld_catalog.py \
  --root /srv/data/ps2 \
  --tool "$PWD/tea-g3-catalog-hash" \
  --cache "$HOME/.cache/tea-g3-catalog.json" \
  --port 18290
```

The helper scans `DVD/`, `CD/` and `APPS/` in the background (60 seconds by default), serves the current catalog at `/catalog`, and provides the app/artwork/VMC endpoints used by TEa.G3. The executable is built from the repository's hash helper source; the legacy Python module filename is retained for existing deployments.

For an existing server installation, omit `--cache` if you intentionally want to continue using its historical cache file. A new cache starts a fresh catalog/hash pass but does not modify game files.

## 4. Network and PS2 settings

Allow these inbound ports on the host firewall:

| Port | Protocol | Purpose |
|---:|---|---|
| 62966 | UDP | Neutrino UDPFS file server |
| 18290 | TCP | TEa.G3 catalog helper |

In `tea_g3/config.ini` on the PS2, set:

```ini
[juegos]
origen = usb, udpfs
servidor = <server IP>
```

Use the server's LAN IP address for `<server IP>`. The USB source can remain enabled alongside UDPFS.

## 5. Scope and safety notes

- UDPFS is a separate source from SMB. Starting these two processes does not enable or prove SMB operation.
- `--read-only` keeps the UDPFS file share read-only; catalog VMC operations are handled separately by the HTTP helper.
- Do not expose ports 62966 or 18290 to the public internet. Restrict them to the trusted LAN.
- Keep the server and PS2 wired. UDPFS streaming is sensitive to packet loss.
- A working host service or a successful CI build does not prove a game boots or streams correctly on a physical PS2. Record ISO/ZSO/UL/SMB/HDD hardware results separately.
