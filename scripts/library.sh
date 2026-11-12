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
case ${1-} in run | diagnose) ;; *) exec cmake --workflow --preset library ;; esac
# Fault prevented: a diagnose call that reads the owner's local settings or an absent directory and reports an empty library as zero failures.
if [ "$1" = diagnose ]; then
  { [ -n "${COGMASTER_ROM_DIR-}" ] && [ -d "$COGMASTER_ROM_DIR" ]; } ||
    { echo 'library: diagnose needs COGMASTER_ROM_DIR to name an existing directory' >&2; exit 2; }
fi
out=${COGMASTER_LIBRARY_OUT:-$root/build/library/results}
[ "$1" != diagnose ] || out=${COGMASTER_LIBRARY_OUT:-$root/build/library/diag}
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
if [ "$1" = diagnose ]; then
  # Fault prevented: counts from a runner that does not match HEAD. The runner is rebuilt unless a test names one; build output is
  # discarded because configure prints the build directory.
  if [ -z "${COGMASTER_LIBRARY_RUN-}" ] && ! (cd "$root" && cmake --preset library && cmake --build --preset library) >/dev/null 2>&1; then
    echo 'library: diagnose could not build the runner' >&2
    exit 1
  fi
  # All scratch lives in one private directory that is removed on exit; a killed process can skip the trap.
  mkdir -p "$root/build/library"
  tmp=$(mktemp -d "$root/build/library/diag.XXXXXX")
  trap 'rm -rf "$tmp"' EXIT
  # core is the first 7 characters of the commit id; dirty means a tracked file outside .planning differs from HEAD.
  core=$(git -C "$root" rev-parse HEAD | cut -c1-7)
  git -C "$root" diff --quiet HEAD -- . ':(exclude).planning' || core=$core+dirty
  behaviour=$(awk '/^#define COGMASTER_BEHAVIOUR_REVISION / {print $3}' "$root/include/cogmaster.h")
  # Fault prevented: totals computed from a ledger split on commas inside titles, or from no ledger at all. The ledger is read only
  # through its own parser.
  if ! sh "$root/scripts/compat-common.sh" cases "$ledger" > "$tmp/cases" 2>/dev/null || ! grep -q '^set(compat_count ' "$tmp/cases"; then
    echo 'library: diagnose could not read the ledger' >&2
    exit 1
  fi
  records=$tmp/records
  if command -v cygpath >/dev/null 2>&1; then records=$(cygpath -m "$records"); fi
  : > "$tmp/records"
  count=0
  for image in "$COGMASTER_ROM_DIR"/* "$COGMASTER_ROM_DIR"/.[!.]* "$COGMASTER_ROM_DIR"/..?*; do
    [ -f "$image" ] && [ ! -L "$image" ] || continue
    case $image in *.[sS][mM][sS]|*.[gG][gG]|*.[bB][iI][nN]|*.[rR][oO][mM]) ;; *) continue ;; esac
    if command -v cygpath >/dev/null 2>&1; then image=$(cygpath -m "$image"); fi
    count=$((count+1))
    COGMASTER_IMAGE=$image COGMASTER_DIAGNOSE=$records cmake -P "$root/scripts/library.cmake" >/dev/null 2>&1 ||
      printf 'other infrastructure_error\n' >> "$tmp/records"
  done
  # Fault prevented: a fail row with no cause group passing as a complete grouping. The reducer prints everything, then exits 1 when
  # the group total differs from the fail total. A record is accepted only when every field matches a closed pattern, and the output
  # is printed from the validated fields, never from the record.
  if awk -v candidates="$count" '
function ok(s, list) { return index(" " list " ", " " s " ") > 0 }
function kv(tok, key) { return index(tok, key "=") == 1 ? substr(tok, length(key) + 2) : "?" }
function isnum(s) { return s ~ /^[0-9][0-9]?[0-9]?[0-9]?$/ }
function isnn(s) { return s == "none" || isnum(s) }
function hex40(s) { return length(s) == 40 && s ~ /^[0-9a-f]*$/ }
BEGIN {
  # The board words are copied from the runner probe; the catalog word compound is not one of them, so a compound row given
  # status unknown would count as malformed.
  boards = "unsupported none sega codemasters korean korean_nb hicom zemina nemesis hes_4pak sega_eeprom"
  kinds = "none unmapped_read unmapped_write rom_write mapper_range ed_undefined unsupported_video_mode unknown"
  devs = "none cpu cart ram mapper ctrl vdp unknown"
  wheres = "rom slot2 ram mapper port cpu vdp unknown none"
  tre = "^[A-Za-z0-9][A-Za-z0-9 ().,\047&:!+_-]*$"
}
{ sub(/\r$/, "") }
FILENAME == ARGV[1] {
  if (split($0, c, "\t") >= 5 && hex40(c[1]) && !(c[1] in title)) title[c[1]] = c[5]
  next
}
NF == 0 { next }
$1 == "other" {
  if (NF != 2) { bad++; next }
  if ($2 == "unsupported") unsup++
  else if ($2 == "malformed_io") mio++
  else if ($2 == "uncatalogued") unc++
  else if ($2 == "excluded") excl++
  else if ($2 == "noprofile") nop++
  else if ($2 == "run_unsupported") { attempted++; unsup++ }
  else if ($2 == "run_error") { attempted++; infra++ }
  else if ($2 == "infrastructure_error") infra++
  else bad++
  next
}
$1 == "rec" {
  attempted++
  v = kv($10, "verdict600"); fd = kv($11, "first_display"); fnb = kv($12, "first_nonblank"); fc = kv($13, "first_change")
  d6 = kv($14, "distinct600"); d18 = kv($15, "distinct1800"); ram = kv($16, "ram"); fk = kv($17, "fault"); dev = kv($18, "dev")
  pc = kv($19, "pc"); ps = kv($20, "pcslot"); wh = kv($21, "where"); ff = kv($22, "fault_frame"); ord = kv($23, "order")
  mp = kv($24, "mapper"); src = kv($25, "source")
  if (NF != 25 || !hex40($2) || !ok($3, "sms sms1 gg") || !ok($4, "export japan") || $5 != "ntsc" || !ok($6, "on off") || !ok($7, boards) ||
      $8 != "diagnose" || $9 != "1" || !ok(v, "pass fail") || !isnn(fd) || !isnn(fnb) || !isnn(fc) || d6 !~ /^[1-8]$/ || d18 !~ /^[1-8]$/ ||
      !ok(ram, "live still") || !ok(fk, kinds) || !ok(dev, devs) || !(pc == "none" || pc ~ /^[0-9A-F][0-9A-F][0-9A-F][0-9A-F]$/) ||
      !(ps == "none" || ps ~ /^[0-4]$/) || !ok(wh, wheres) || !ok(ff, "0 1-9 10-99 100-599 600+ none") ||
      !ok(ord, "before after nopicture none") || !ok(mp, boards) || !ok(src, "none db header heuristic default")) { bad++; next }
  evaluated++
  if (v == "pass") next
  failed++
  hasf = (ff != "none" && ff != "600+"); hasd = (fd != "none" && fd + 0 <= 600); hasp = (d6 + 0 >= 2)
  g = ""
  if (mp != $7) g = "input_mismatch"
  else if (hasf && !hasd) g = "fault_no_display." fk "." dev
  else if (hasf && !hasp) g = "fault_static." fk "." dev
  else if (hasf) g = "fault_alive." fk "." dev
  else if (!hasd) g = (fnb == "none") ? "dark_no_fault.never_draws" : ((fnb + 0 <= 600) ? "dark_no_fault.latch_miss" : "dark_no_fault.late")
  else if (!hasp) g = (d18 + 0 >= 2) ? "static_no_fault.animates_late" : ((ram == "still") ? "static_no_fault.frozen" : "static_no_fault.alive_still")
  if (g == "") next
  groups[g]++; grouped++
  t = title[$2]
  if (length(t) > 80 || t !~ tre) { t = "-"; withheld++ }
  printf "D diag: title=%s group=%s first_display=%s first_nonblank=%s distinct600=%s distinct1800=%s fault=%s dev=%s pc=%s mapper=%s first_change=%s fault_frame=%s order=%s where=%s ram=%s source=%s\n", t, g, fd, fnb, d6, d18, fk, dev, pc, mp, fc, ff, ord, wh, ram, src
  next
}
{ bad++ }
END {
  for (g in groups) printf "G %d %s\n", groups[g], g
  if (!("input_mismatch" in groups)) print "G 0 input_mismatch"
  printf "O diag-other: candidates=%d attempted=%d evaluated=%d malformed=%d malformed_io=%d uncatalogued=%d skipped_excluded=%d noprofile=%d unsupported=%d infrastructure_error=%d title_withheld=%d\n", candidates, attempted, evaluated, bad, mio, unc, excl, nop, unsup, infra, withheld
  printf "O diag-other: judged=%d run_fail=%d groups_total=%d fail_total=%d\n", evaluated, failed, grouped, failed
  exit grouped != failed
}' "$COGMASTER_CATALOG" "$tmp/records" > "$tmp/lines" 2>/dev/null; then rc=0; else rc=$?; fi
  [ "$rc" -le 1 ] || { echo 'library: diagnose could not read its records' >&2; exit 1; }
  {
    printf 'library-diagnose 1 core=%s behaviour=%s\n' "$core" "$behaviour"
    sed -n 's/^D //p' "$tmp/lines" | sort
    sed -n 's/^G //p' "$tmp/lines" | sort -k1,1nr -k2,2 | sed 's/^\([0-9][0-9]*\) \(.*\)$/diag-group: leaf=\2 rows=\1/'
    sed -n 's/^O //p' "$tmp/lines"
  } > "$tmp/out"
  cat "$tmp/out" > "$out/diagnose.txt"
  cat "$tmp/out"
  exit "$rc"
fi
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
