#!/bin/sh
# Local operation only: never print filenames, probes or guest observations to CI/evidence.
set -eu
export LC_ALL=C
root=$(git rev-parse --show-toplevel)
if [ -z "${COGMASTER_ROM_DIR-}" ] && [ -f "$root/.env.local" ]; then
  # Treat the dotenv value as literal data, never source executable shell from it.
  COGMASTER_ROM_DIR=$(sed -n 's/^COGMASTER_ROM_DIR=//p' "$root/.env.local")
  export COGMASTER_ROM_DIR
fi
if [ "${1-}" != run ]; then
  exec cmake --workflow --preset library
fi
out=${COGMASTER_LIBRARY_OUT:-$root/build/library/results}
case $out in "$root"/build/*) ;; *) echo 'library: results must stay inside ignored build output' >&2; exit 2 ;; esac
export COGMASTER_LIBRARY_OUT=$out
mkdir -p "$out"
physical=$(cd "$out" && pwd -P)
root_physical=$(cd "$root" && pwd -P)
case $physical in "$root_physical"/build/*) ;; *) exit 2 ;; esac
chmod 700 "$out"
umask 077
# The tracked ledger, page and catalog are overridable so tests never touch tracked files.
ledger=${COGMASTER_LEDGER:-$root/tests/compat/ledger.csv}
page=${COGMASTER_PAGE:-$root/docs/compatibility.md}
COGMASTER_CATALOG=${COGMASTER_CATALOG:-$root/tests/compat/catalog.tsv}
# Fault prevented: a judge that reads its catalog and reasons from the working directory, which the owner may run from anywhere.
export COGMASTER_CATALOG
export COMPAT_CATALOG=$COGMASTER_CATALOG COMPAT_REASONS=$root/tests/compat/reasons.txt
sh "$root/scripts/compat-common.sh" cases "$ledger" > "$out/cases.cmake"
: > "$out/profiles.csv"
printf 'candidate,status,evaluated,attempted\n' > "$out/results.csv"
printf 'id,title,machine,region,tv,mapper,predicate,status,evidence,date\n' > "$out/public-facts.csv"
COGMASTER_LIBRARY_DATE=$(date -u +%F)
COGMASTER_LIBRARY_REV=$(git rev-parse HEAD)
export COGMASTER_LIBRARY_DATE COGMASTER_LIBRARY_REV
count=0
# One flat directory, byte order, regular files only; no recursion or symlink traversal.
for image in "$COGMASTER_ROM_DIR"/* "$COGMASTER_ROM_DIR"/.[!.]* "$COGMASTER_ROM_DIR"/..?*; do
  [ -f "$image" ] && [ ! -L "$image" ] || continue
  case $image in *.[sS][mM][sS]|*.[gG][gG]|*.[bB][iI][nN]|*.[rR][oO][mM]) ;; *) continue ;; esac
  if command -v cygpath >/dev/null 2>&1; then image=$(cygpath -m "$image"); fi
  count=$((count+1))
  COGMASTER_IMAGE=$image COGMASTER_CANDIDATE=$count cmake -P "$root/scripts/library.cmake" > "$out/$count.judge.txt" 2>&1 ||
    printf '%s,infrastructure_error,0,0\n' "$count" >> "$out/results.csv"
done
awk -F, 'NR>1 {c++; e+=($3>0); p+=$3; a+=$4; s[$2]++} END {printf "library: candidates=%d attempted=%d evaluated=%d coverage=%d predicates=%d\n",c,a,e,e,p; for(k in s) print "library: " k "=" s[k]}' "$out/results.csv" | sort
# Counts only: no name, path, probe value or hash is printed. profiles.csv rows are kind,region,tv,fm,status.
{
  echo 'library-judge 1'
  awk -F, '$1=="sms" && $2=="export" && $3=="ntsc" {c++} $3=="pal" {r++; p+=($5=="pass"); f+=($5=="fail")} END {if(c+r) printf "pal: carts=%d ran_pal=%d boots_pass=%d boots_fail=%d\n",c,r,p,f; else print "pal: skipped: not configured"}' "$out/profiles.csv"
  awk -F, '$2=="japan" && $4=="on" {c++; p+=($5=="pass"); f+=($5=="fail")} END {if(c) printf "fm: carts=%d boots_pass=%d boots_fail=%d\n",c,p,f; else print "fm: skipped: not configured"}' "$out/profiles.csv"
} > "$out/summary.txt"
# Fault prevented: a run rewriting the tracked ledger and page when the page cannot hold the rows, or past the ratchet.
cap=${COGMASTER_DOCS_CAP:-$(sed -n 's/.*"docs.max_bytes": *\([0-9][0-9]*\).*/\1/p' "$root/quality.json")}
rev=$(awk '/^#define COGMASTER_BEHAVIOUR_REVISION / {print $3}' "$root/include/cogmaster.h")
: > "$out/allow.txt"
awk '!seen[$0]++' "$out/public-facts.csv" > "$out/facts.csv"
merge=held
if COMPAT_REV=$COGMASTER_LIBRARY_REV sh "$root/scripts/compat-common.sh" update "$ledger" "$out/facts.csv" > "$out/next-ledger.csv" 2>/dev/null; then
  added=$(($(wc -l < "$out/next-ledger.csv") - $(wc -l < "$ledger")))
  if sh "$root/scripts/compat-ratchet.sh" "$ledger" "$out/next-ledger.csv" "$out/allow.txt" "$COGMASTER_LIBRARY_DATE" "$rev" "$rev" >/dev/null 2>&1; then
    sh "$root/scripts/compat-common.sh" docs "$out/next-ledger.csv" > "$out/next-page.md"
    bytes=$(($(wc -c < "$out/next-page.md")))
    if [ "$bytes" -lt "$cap" ]; then
      cmp -s "$ledger" "$out/next-ledger.csv" || cp "$out/next-ledger.csv" "$ledger"
      cmp -s "$page" "$out/next-page.md" || cp "$out/next-page.md" "$page"
      echo "merge: rows_added=$added ratchet=ok page_bytes=$bytes" >> "$out/summary.txt"
    else
      echo 'merge: held (page over the docs cap)' >> "$out/summary.txt"
    fi
  else
    echo 'merge: held (ratchet refused)' >> "$out/summary.txt"
    merge=refused
  fi
else
  echo 'merge: held (update refused)' >> "$out/summary.txt"
  merge=refused
fi
# Fault prevented: an owner's run whose merge says nothing about whether the experimental flag must now leave, or a printed failure
# mistaken for a release gate: the command's status is printed and never this script's. Counts only; a stand-in catalog has no pin counts.
counts=-DCOUNTS=none
[ "$COGMASTER_CATALOG" != "$root/tests/compat/catalog.tsv" ] || counts=
if cmake -DLEDGER="$ledger" -DINFO="$root/libretro/cogmaster_libretro.info.in" -DCATALOG="$COGMASTER_CATALOG" $counts \
  -DOUT="$out/exit" -P "$root/tests/compat/exit.cmake" > "$out/exit.txt" 2>&1; then status=0; else status=$?; fi
sed -n 's/^-- \(exit: .*\)$/\1/p' "$out/exit.txt" >> "$out/summary.txt"
if [ "$status" -ne 0 ]; then
  echo "exit: failed ($status)" >> "$out/summary.txt"
  sed -n 's/^  \(exit: .*\)$/\1/p' "$out/exit.txt" | sed -n 1p >> "$out/summary.txt"
fi
cat "$out/summary.txt"
[ "$merge" != refused ] || exit 1
