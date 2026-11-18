#!/usr/bin/env python3
"""site/changelog.html from docs/release-v*.md, newest first. Run it before a release: python3 tools/changelog.py
Only what changed is kept: the sections on updating, checksums and the quick start stay in the release notes.
A version with no git tag yet is marked as not released. Markdown: just what the notes use (## headings, lists,
paragraphs, fenced code, **bold**, `code`, [links](url), bare URLs).
"""
import html
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SKIP = re.compile(r"updating|checksums|quick start", re.I)


def inline(s):
    s = html.escape(s, quote=False)
    s = re.sub(r"`([^`]+)`", r"<code>\1</code>", s)
    s = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", s)
    s = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", r'<a href="\2">\1</a>', s)
    return re.sub(r"(?<![\"'>])(https?://[^\s<)]+[^\s<).,])", r'<a href="\1">\1</a>', s)


def body(md):
    out, skip, lines, i = [], False, md.splitlines(), 0
    while i < len(lines):
        l = lines[i]
        if l.startswith("## "):
            skip = bool(SKIP.search(l))
            if not skip: out.append(f"<h3>{inline(l[3:])}</h3>")
            i += 1
        elif skip or not l.strip() or l.startswith("!["):
            i += 1
        elif l.startswith("```"):
            j = i + 1
            while j < len(lines) and not lines[j].startswith("```"): j += 1
            out.append("<pre>" + html.escape("\n".join(lines[i + 1:j])) + "</pre>")
            i = j + 1
        elif re.match(r"(-|\d+\.) ", l):
            tag, items = ("ul" if l[0] == "-" else "ol"), []
            while i < len(lines) and (re.match(r"(-|\d+\.) ", lines[i]) or lines[i].startswith("  ")):
                if lines[i].startswith("  "): items[-1] += " " + lines[i].strip()
                else: items.append(re.sub(r"^(-|\d+\.) ", "", lines[i]))
                i += 1
            out.append(f"<{tag}>" + "".join(f"<li>{inline(x)}</li>" for x in items) + f"</{tag}>")
        else:
            j = i
            while j < len(lines) and lines[j].strip() and not re.match(r"(## |```|- |\d+\. )", lines[j]): j += 1
            out.append("<p>" + inline(" ".join(x.strip() for x in lines[i:j])) + "</p>")
            i = j
    return "\n      ".join(out)


def main():
    notes = sorted(ROOT.glob("docs/release-v*.md"), key=lambda p: [int(n) for n in p.stem[9:].split(".")], reverse=True)
    parts = []
    for p in notes:
        v = p.stem[9:]
        r = subprocess.run(["git", "-C", str(ROOT), "log", "-1", "--format=%as", "refs/tags/v" + v], capture_output=True, text=True)
        when = r.stdout.strip() if r.returncode == 0 else "not released yet"
        link = f' · <a href="https://github.com/danielnuld/nuld/releases/tag/v{v}">download</a>' if r.returncode == 0 else ""
        parts.append(f'''  <section class="chapter" id="v{v}">
    <header><b>{v}</b><h2 class="mono">{when}{link}</h2></header>
    <div>
      {body(p.read_text(encoding="utf-8"))}
    </div>
  </section>
''')
    page = (ROOT / "tools/changelog.html").read_text(encoding="utf-8").replace("<!-- versions -->\n", "\n".join(parts))
    (ROOT / "site/changelog.html").write_text(page, encoding="utf-8", newline="\n")
    print(len(notes), "versions")


if __name__ == "__main__":
    md = "Text with `code`, **bold** and [a link](https://x.y/z).\n\n## New\n\n- one\n  more\n- two\n\n## Updating\n\ngone\n"
    assert body(md) == ('<p>Text with <code>code</code>, <b>bold</b> and <a href="https://x.y/z">a link</a>.</p>\n      '
                        "<h3>New</h3>\n      <ul><li>one more</li><li>two</li></ul>"), body(md)
    main()
