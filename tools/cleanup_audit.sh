#!/usr/bin/env bash
# Read-only inventory of legacy NULD/ORBIT identifiers. Review results manually; this script never edits files.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 1

section() {
    printf '\n============================================================\n'
    printf '%s\n' "$1"
    printf '============================================================\n'
}

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Run this audit inside a Git working tree."
    exit 2
fi

section "1) All tracked NULD / ORBIT references (case-insensitive)"
echo "Review each hit. Historical notes, compatibility code and server tool names may be intentional."
git grep -n -I -i -E 'NULD|ORBIT' -- ':!third_party/**' || true

section "2) Possible user-visible C / header / script string literals"
echo "These are candidates for manual review, not automatic replacements."
git grep -n -I -i -E '"[^"]*(NULD|ORBIT|nuld|orbit)[^"]*"' -- '*.c' '*.h' '*.py' '*.txt' '*.md' ':!third_party/**' || true

section "3) Compatibility paths and migration references to preserve"
git grep -n -I -i -E 'BOOT/(NULD|ORBIT)\.ELF|VMC/(NULD|ORBIT)\.bin|/nuld/|/orbit/' -- ':!third_party/**' || true

section "4) Tracked filenames containing NULD / ORBIT"
git ls-files | grep -iE '(^|/)[^/]*(nuld|orbit)[^/]*$' || true

section "5) Safety reminder"
cat <<'EOF'
Do not bulk-rename or delete:
- BOOT/NULD.ELF and BOOT/ORBIT.ELF: legacy FreeMcBoot entry points.
- VMC/NULD.bin and VMC/ORBIT.bin: existing shared virtual memory cards.
- nuld/ and orbit/: legacy data directories used during migration.
- tools/nuld_catalog.py and tools/nuld_hash.c: server-side compatibility helpers.
- Internal theme keys "nuld" and "orbit": preserve them while their visible labels remain TEa.G3 / Classic / Orbit as appropriate.

This report is an inventory only. Decide each occurrence by its runtime purpose.
EOF
