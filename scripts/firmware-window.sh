#!/bin/sh
# Faults prevented: a 32-byte window of the owner's firmware dump inside a tree file, build output or release zip that could
# be committed or shipped, and a silent pass (no dump configured prints 0 compared). Prints ordinals, sizes and target names.
# Usage: firmware-window.sh [DUMPDIR [FILE...]]. Not covered: base64, decimal lists, XOR and compression.
set -eu; export LC_ALL=C
cd "$(git rev-parse --show-toplevel)"
D=${1:-${COGMASTER_BIOS_DIR-}}; [ $# -gt 0 ] && shift
[ -z "$D" ] && [ -f .env.local ] && D=$(sed -n 's/^COGMASTER_BIOS_DIR=//p' .env.local)
[ -n "$D" ] && [ -d "$D" ] || { echo 'firmware window: no dump configured, 0 compared'; exit 0; }
T=$(mktemp -d "${TMPDIR:-/tmp}/fwwin.XXXXXX"); trap 'rm -rf "$T"' EXIT
targets() {
  if [ $# -gt 0 ]; then printf '%s\n' "$@"; return; fi
  git ls-files --cached --others --exclude-standard | grep -Ev '^(bios|roms|private)/' || true
  printf '%s\n' build/*/libcogmaster.a build/*/cogmaster-run* build/*/cogmaster_libretro.*
  for z in build/*/cogmaster-*.zip; do [ -f "$z" ] && mkdir -p "$T/$z" && unzip -qo "$z" -d "$T/$z" && find "$T/$z" -type f || true; done
}
{
  for f in "$D"/*; do # a dump is a regular file of at most 256 KiB; anything else is counted and ignored
    if [ -f "$f" ] && [ ! -L "$f" ] && [ "$(wc -c < "$f")" -le 262144 ]; then echo N; od -An -v -tx1 "$f"; else echo I; fi
  done
  targets "$@" | while IFS= read -r t; do
    [ -f "$t" ] || continue
    case $PWD/$t in "$D"/*) continue ;; esac
    printf 'T %s\n' "${t#"$T"/}"; od -An -v -tx1 "$t"
    # a text target is also reduced to its byte literals: each even-length run of hex digits (0x41, $41, 4142), in order
    if grep -Iq . "$t"; then printf 'T %s\n' "${t#"$T"/}"; tr -cs '0-9a-fA-F' '\n' < "$t" | grep -E '^([0-9a-fA-F]{2})+$' | tr A-F a-f | fold -w 32; fi
  done
} | awk '$1=="I" {ig++; next} $1=="N" {nd++; d=1; win=""; n=0; delete c; u=0; next} $1=="T" {t=substr($0,3); d=0; win=""; n=0; next}
  # every other line holds hex bytes, rolled through a 32-byte window (u = distinct values in a dump window)
  { for (k=1; k<=NF; k++) for (j=1; j<=length($k); j+=2) { b=substr($k,j,2)
      if (n<32) n++; else { if (d && --c[substr(win,1,2)]==0) u--; win=substr(win,3) }
      win=win b
      if (d) { sz[nd]++; if (c[b]++==0) u++; if (n==32 && u>=8) W[win]=nd }
      else if (n==32 && (win in W) && !(t in hit)) { hit[t]=bad=1; printf "firmware window: dump %d (%d bytes) matches %s\n", W[win], sz[W[win]], t } } }
  END { if (!bad) printf "firmware window: %d compared, %d ignored\n", nd, ig; exit bad }'
