#!/bin/sh
# Faults prevented: a 32-byte window of the owner's firmware dump inside a tree file, build output or release zip that could
# be committed or shipped, and a silent pass: only no dump configured says 0 compared and exits 0; a configured directory
# that is unusable or holds no dump, an unreadable target and a zip that will not open all fail. Prints ordinals, sizes and
# target names. Usage: firmware-window.sh [DUMPDIR [FILE...]]. Not covered: base64, decimal lists, XOR, compression,
# xxd and hexdump -C layouts, and a dump kept as a zip.
set -eu; export LC_ALL=C
cd "$(git rev-parse --show-toplevel)"
D=${1:-${COGMASTER_BIOS_DIR-}}; [ $# -gt 0 ] && shift
[ -z "$D" ] && [ -f .env.local ] && D=$(sed -n 's/^COGMASTER_BIOS_DIR=//p' .env.local)
[ -n "$D" ] || { echo 'firmware window: no dump configured, 0 compared'; exit 0; }
[ -d "$D" ] || { echo 'firmware window: the configured directory is not usable' >&2; exit 2; }
T=$(mktemp -d "${TMPDIR:-/tmp}/fwwin.XXXXXX"); trap 'rm -rf "$T"' EXIT
rd() { od -An -v -tx1 "$1" 2>/dev/null || echo "$2" >> "$T/u"; } # hex of $1; a failure is recorded under the name $2
targets() {
  if [ $# -gt 0 ]; then printf '%s\n' "$@"; return; fi
  git ls-files -z --cached --others --exclude-standard | tr '\0' '\n' | grep -Ev '^(bios|roms|private)/' || true
  printf '%s\n' build/*/libcogmaster.a build/*/cogmaster-run* build/*/libretro/cogmaster_libretro.*
  for z in build/*/cogmaster-*.zip; do
    [ -f "$z" ] && { mkdir -p "$T/$z" && unzip -qo "$z" -d "$T/$z" 2>/dev/null && find "$T/$z" -type f || echo "$z" >> "$T/u"; }
  done
}
{
  for f in "$D"/*; do # a dump is a regular file of at most 256 KiB; anything else is counted and ignored
    if [ -f "$f" ] && [ ! -L "$f" ] && [ "$(wc -c < "$f")" -le 262144 ]; then echo N; rd "$f" dump; else echo I; fi
  done
  targets "$@" | while IFS= read -r t; do
    [ -f "$t" ] || continue
    case $PWD/$t in "$D"/*) continue ;; esac
    printf 'T %s\n' "${t#"$T"/}"; rd "$t" "${t#"$T"/}"
    # a text target is also reduced to its byte literals: each even-length run of hex digits (0x41, $41, 4142), in order
    if grep -Iq . "$t"; then printf 'T %s\n' "${t#"$T"/}"; tr -cs '0-9a-fA-F' '\n' < "$t" | grep -E '^([0-9a-fA-F]{2})+$' | tr A-F a-f | fold -w 32; fi
  done
  echo Z
} | awk '$1=="Z" {z=1; next} $1=="I" {ig++; next} $1=="N" {nd++; d=1; win=""; n=0; delete c; u=0; next} $1=="T" {t=substr($0,3); d=0; win=""; n=0; next}
  # every other line holds hex bytes, rolled through a 32-byte window (u = distinct values in a dump window)
  { for (k=1; k<=NF; k++) for (j=1; j<=length($k); j+=2) { b=substr($k,j,2)
      if (n<32) n++; else { if (d && --c[substr(win,1,2)]==0) u--; win=substr(win,3) }
      win=win b
      if (d) { sz[nd]++; if (c[b]++==0) u++; if (n==32 && u>=8) W[win]=nd }
      else if (n==32 && (win in W) && !(t in hit)) { hit[t]=bad=1; printf "firmware window: dump %d (%d bytes) matches %s\n", W[win], sz[W[win]], t } } }
  END { if (!z) { print "firmware window: scan incomplete"; exit 2 }
    if (!bad && !nd) { print "firmware window: no usable dump file in the directory"; exit 2 }
    if (!bad) printf "firmware window: %d compared, %d ignored\n", nd, ig; exit bad }' && rc=0 || rc=$?
# a target that could not be read or opened is named and never passes, but the scan has gone on to the later targets
if [ -s "$T/u" ]; then sed 's/^/firmware window: cannot read /' "$T/u" >&2; [ "$rc" -ne 0 ] || rc=2; fi
exit "$rc"
