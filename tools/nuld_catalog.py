#!/usr/bin/env python3
"""nuld-catalog: the game server's list of PS2 games for the NULD launcher (phase 17).

GET  /catalog        one line per ISO under <root>/DVD and <root>/CD, tab-separated:
                     path  size  serial  hash  title   (serial SLUS_213.76 form, hash = rcheevos rc_hash_ps2,
                     "-" while not hashed yet or unreadable). Answered from the cache, never waiting for a hash:
                     a background thread hashes new or changed ISOs (key: path, size, mtime) with nuld_hash.
                     Then the apps under <root>/APPS, OPL layout (a loose .elf, or a folder with title.cfg "title=" and
                     "boot="): path APPS/<URL-quoted path under APPS>, serial and hash "-".
GET  /app/<quoted path>  the app's ELF (the launcher downloads it to its USB and runs it)
GET  /art/<quoted name>  <root>/ART/<name>, OPL's art next to the games (the launcher asks for <SERIAL>_COV.png|jpg
                     and <ELF>_COV.png|jpg of the server's games and apps that have no cover yet)
POST /vmc/<name>     creates <root>/VMC/<name>.bin (empty 8 MB card, the launcher's format) if missing.
GET  /vmc/<name>/list               the card's root, one "name<TAB>mode<TAB>modified hex" line per entry
GET  /vmc/<name>/file/<dir>/<file>  one file of a save (the launcher's save icon, phase 20)

Runs as a user service on nuld next to udpfs_server.py: tools/nuld_catalog.py --root /srv/data/ps2 --port 18290
"""
import argparse, json, os, re, subprocess, threading, time
from urllib.parse import quote, unquote
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

OPL_PREFIX = re.compile(r"^[A-Z]{4}_\d{3}\.\d{2}\.")  # "SLUS_209.46.God of War.iso" (OPL naming)


def title_of(name):  # as the launcher's iso_title
    t = name[12:] if OPL_PREFIX.match(name) else name
    root, ext = os.path.splitext(t)
    return root if ext.lower() in (".iso", ".vcd", ".elf") else t


class Catalog:
    def __init__(self, root, tool, cache):
        self.root, self.tool, self.cache_path = root, tool, cache
        self.lock = threading.Lock()
        try:
            self.cache = json.load(open(cache))
        except (OSError, ValueError):
            self.cache = {}  # path -> [size, mtime, serial, hash]

    def isos(self):
        for d in ("DVD", "CD"):
            try:
                names = sorted(os.listdir(os.path.join(self.root, d)))
            except OSError:
                continue
            for n in names:
                p = os.path.join(d, n)
                if n.lower().endswith(".iso") and os.path.isfile(os.path.join(self.root, p)):
                    st = os.stat(os.path.join(self.root, p))
                    yield p, st.st_size, int(st.st_mtime)

    def apps(self):  # (path under APPS, title) per app, as the launcher's load_games reads its USB
        d = os.path.join(self.root, "APPS")
        try:
            names = sorted(os.listdir(d))
        except OSError:
            return
        for n in names:
            p = os.path.join(d, n)
            if n.lower().endswith(".elf") and os.path.isfile(p):
                yield n, title_of(n)
            elif os.path.isfile(os.path.join(p, "title.cfg")):
                cfg = {}
                for line in open(os.path.join(p, "title.cfg"), encoding="utf-8", errors="replace"):
                    k, _, v = line.partition("=")
                    cfg[k.strip().lower()] = v.strip()
                if cfg.get("boot") and os.path.isfile(os.path.join(p, cfg["boot"])):
                    yield n + "/" + cfg["boot"], cfg.get("title") or n

    def app_get(self, path):  # (status, bytes) for GET /app/<quoted path>: only what apps() lists
        rel = unquote(path[len("/app/"):])
        if rel not in {a for a, _ in self.apps()}:
            return 404, b""
        return 200, open(os.path.join(self.root, "APPS", rel), "rb").read()

    def art_get(self, path):  # (status, bytes) for GET /art/<quoted name>: one file of <root>/ART, no subfolders
        name = unquote(path[len("/art/"):])
        p = os.path.join(self.root, "ART", name)
        if "/" in name or "\\" in name or name.startswith(".") or not os.path.isfile(p):
            return 404, b""
        return 200, open(p, "rb").read()

    def text(self):
        out = []
        with self.lock:
            for p, size, mtime in self.isos():
                c = self.cache.get(p)
                serial, h = (c[2], c[3]) if c and c[0] == size and c[1] == mtime else ("-", "-")
                out.append(f"{p}\t{size}\t{serial}\t{h}\t{title_of(os.path.basename(p))}\n")
        for a, title in self.apps():
            size = os.path.getsize(os.path.join(self.root, "APPS", a))
            out.append(f"APPS/{quote(a)}\t{size}\t-\t-\t{title}\n")
        return "".join(out)

    def refresh(self):  # hash what is new or changed; drop what is gone
        todo = [(p, s, m) for p, s, m in self.isos() if (c := self.cache.get(p)) is None or c[:2] != [s, m]]
        for p, size, mtime in todo:
            r = subprocess.run([self.tool, "hash", os.path.join(self.root, p)], capture_output=True, text=True)
            serial, h = (r.stdout.split("\t") + ["-", "-"])[:2] if r.stdout else ("-", "-")
            with self.lock:
                self.cache[p] = [size, mtime, serial.strip(), h.strip()]
            print(f"hashed {p}: {serial.strip()} {h.strip()}", flush=True)
        with self.lock:
            present = {p for p, _, _ in self.isos()}
            for p in [p for p in self.cache if p not in present]:
                del self.cache[p]
            if todo:
                tmp = self.cache_path + ".tmp"
                json.dump(self.cache, open(tmp, "w"))
                os.replace(tmp, self.cache_path)

    def vmc_get(self, path):  # (status, bytes) for GET /vmc/<name>/list or /vmc/<name>/file/<dir>/<file>
        m = re.fullmatch(r"/vmc/([A-Za-z0-9_.-]+)/(list|file/([^/]+)/([^/]+))", path)
        if not m or ".." in path:
            return 404, b""
        card = os.path.join(self.root, "VMC", m.group(1) + ".bin")
        if not os.path.exists(card):
            return 404, b""
        args = ["vmc-list", card] if m.group(2) == "list" else \
            ["vmc-read", card, unquote(m.group(3)) + "/" + unquote(m.group(4))]
        r = subprocess.run([self.tool] + args, capture_output=True)
        return (200, r.stdout) if r.returncode == 0 else (404, b"")

    def vmc(self, name):
        if not re.fullmatch(r"[A-Za-z0-9_.-]{1,32}", name) or name.startswith("."):
            return 400
        path = os.path.join(self.root, "VMC", name + ".bin")
        old = os.path.join(self.root, "VMC", "ORBIT.bin")  # pre-NULD shared card
        nuld = os.path.join(self.root, "VMC", "NULD.bin")  # previous TEa.G3 release
        if name == "TEa.G3" and not os.path.exists(path):
            legacy = nuld if os.path.exists(nuld) else old
            if os.path.exists(legacy):
                os.replace(legacy, path)
        elif name == "NULD" and not os.path.exists(path) and os.path.exists(old):
            os.replace(old, path)
        if os.path.exists(path):
            if os.path.getsize(path) == 8 * 1024 * 1024:
                return 200
            # a card cut short (a write over udpfs that stalled, 2026-10-03) is no card: kept aside, never deleted
            os.replace(path, path + time.strftime(".short-%Y%m%d-%H%M%S"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        return 201 if subprocess.run([self.tool, "vmc", path]).returncode == 0 else 500


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default="/srv/data/ps2")
    ap.add_argument("--port", type=int, default=18290)
    ap.add_argument("--tool", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "nuld_hash"))
    ap.add_argument("--cache", default=os.path.expanduser("~/.cache/nuld-catalog.json"))
    ap.add_argument("--rescan", type=int, default=60, help="seconds between scans for new ISOs")
    a = ap.parse_args()
    os.makedirs(os.path.dirname(a.cache), exist_ok=True)
    cat = Catalog(a.root, a.tool, a.cache)

    def scanner():
        while True:
            try:
                cat.refresh()
            except Exception as e:  # keep serving the last good list
                print("scan failed:", e, flush=True)
            time.sleep(a.rescan)

    threading.Thread(target=scanner, daemon=True).start()

    class H(BaseHTTPRequestHandler):
        def send(self, code, body=b""):
            self.send_response(code)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            if self.path == "/catalog":
                self.send(200, cat.text().encode())
            elif self.path.startswith("/app/"):
                self.send(*cat.app_get(self.path))
            elif self.path.startswith("/art/"):
                self.send(*cat.art_get(self.path))
            else:
                self.send(*cat.vmc_get(self.path))

        def do_POST(self):
            m = re.fullmatch(r"/vmc/([^/]+)", self.path)
            self.send(cat.vmc(m.group(1)) if m else 404)

        def log_message(self, fmt, *args):
            print("%s %s" % (self.client_address[0], fmt % args), flush=True)

    ThreadingHTTPServer(("0.0.0.0", a.port), H).serve_forever()


if __name__ == "__main__":
    main()
