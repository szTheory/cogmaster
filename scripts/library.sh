#!/bin/sh
# Local operation only: never print filenames, paths, probe values or digests; diagnose prints counts, enum words, frame numbers, fault pcs and catalog titles.
set -eu
export LC_ALL=C
root=$(git rev-parse --show-toplevel)
if [ -z "${COGMASTER_ROM_DIR-}" ] && [ -f "$root/.env.local" ]; then
  # Treat the dotenv value as literal data, never source executable shell from it.
  COGMASTER_ROM_DIR=$(sed -n 's/^COGMASTER_ROM_DIR=//p' "$root/.env.local")
  export COGMASTER_ROM_DIR
fi
case ${1-} in run | diagnose) ;; *) exec cmake --workflow --preset library ;; esac
# Fault prevented: a diagnose call over an unset or absent directory reporting an empty library as zero failures.
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
  # All scratch lives in one private directory that is removed on exit. A shell such as dash skips the EXIT trap on a
  # signal, so INT, TERM and HUP exit explicitly; only SIGKILL leaves the directory behind.
  mkdir -p "$root/build/library"
  tmp=$(mktemp -d "$root/build/library/diag.XXXXXX")
  trap 'rm -rf "$tmp"' EXIT
  trap 'exit 130' INT
  trap 'exit 143' TERM
  trap 'exit 129' HUP
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
function idx(s, list,   n, a, i) { n = split(list, a, " "); for (i = 1; i <= n; i++) if (a[i] == s) return i - 1; return 0 }
function hex40(s) { return length(s) == 40 && s ~ /^[0-9a-f]*$/ }
BEGIN {
  # The board words are copied from the runner probe; the catalog word compound is not one of them, so a compound row given
  # status unknown would count as malformed.
  boards = "unsupported none sega codemasters korean korean_nb hicom zemina nemesis hes_4pak sega_eeprom"
  kinds = "none unmapped_read unmapped_write rom_write mapper_range ed_undefined unsupported_video_mode unknown"
  devs = "none cpu cart ram mapper ctrl vdp unknown"
  wheres = "rom slot2 ram mapper port cpu vdp unknown none"
  tre = "^[A-Za-z0-9][A-Za-z0-9 ().,\047&:!+_-]*$"
  split("input_mismatch fault_no_display fault_static fault_alive dark_no_fault static_no_fault", cn, " ")
}
{ sub(/\r$/, "") }
FILENAME == ARGV[1] {
  if (split($0, c, "\t") >= 5 && hex40(c[1]) && !(c[1] in title)) title[c[1]] = c[5]
  next
}
# The ledger as compat-common.sh cases prints it: set(compat_N_field [[value]]); never split on commas.
FILENAME == ARGV[2] {
  if ($0 !~ /^set\(compat_[0-9]+_[a-z0-9_]+ \[\[.*\]\]\)$/) next
  s = substr($0, 12); n = s; sub(/_.*/, "", n); s = substr(s, length(n) + 2); f = s; sub(/ .*/, "", f)
  L[n + 0, f] = substr(s, length(f) + 4, length(s) - length(f) - 6)
  if (n + 0 > ln) ln = n + 0
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
  mp = kv($24, "mapper"); src = kv($25, "source"); vm = kv($26, "vdpmode")
  fa = kv($27, "faddr"); p2 = kv($28, "pow2"); wc = kv($29, "wclass"); pcl = kv($30, "pclass"); sz = kv($31, "size"); hd = kv($32, "header")
  if (NF != 32 || !hex40($2) || !ok($3, "sms sms1 gg") || !ok($4, "export japan") || $5 != "ntsc" || !ok($6, "on off") || !ok($7, boards) ||
      $8 != "diagnose" || $9 != "1" || !ok(v, "pass fail") || !isnn(fd) || !isnn(fnb) || !isnn(fc) || d6 !~ /^[1-8]$/ || d18 !~ /^[1-8]$/ ||
      !ok(ram, "live still") || !ok(fk, kinds) || !ok(dev, devs) || !(pc == "none" || pc ~ /^[0-9A-F][0-9A-F][0-9A-F][0-9A-F]$/) ||
      !(ps == "none" || ps ~ /^[0-4]$/) || !ok(wh, wheres) || !ok(ff, "0 1-9 10-99 100-599 600+ none") ||
      !ok(ord, "before after nopicture none") || !ok(mp, boards) || !ok(src, "none db header heuristic default") ||
      !ok(vm, "none graphics1 text graphics2 mixed multicolor mode4 invalid sms2only") || !ok(fa, "fffc fffd-f other none") || !ok(p2, "y n") ||
      !ok(wc, "reg-0 reg-zm reg-cm reg-korean reg-4pak lt100 rom slot2 high none") || !ok(pcl, "00-06 3e-3f lt40 40-bf high none") ||
      !ok(sz, "le16k le32k le48k le64k le128k le256k le512k gt512k") || !ok(hd, "none valid bad")) { bad++; next }
  evaluated++
  key = $2 SUBSEP $3 SUBSEP $4 SUBSEP $5 SUBSEP $6
  if (v == "pass") {
    runpass[key] = 1
    hist[mp SUBSEP (fc == "none" ? 999999 : int((fc - 1) / 60) * 60 + 1)]++
    next
  }
  failed++; runfail[key] = 1; rf[failed] = key
  hasf = (ff != "none" && ff != "600+"); hasd = (fd != "none" && fd + 0 <= 600); hasp = (d6 + 0 >= 2)
  # cls is the class precedence 1 to 6 and sp the split order; both are sort keys for the leaf order, with the kind and device codes.
  g = ""; cls = 0; sp = 0
  if (mp != $7) { g = "input_mismatch"; cls = 1 }
  else if (hasf && !hasd) { g = "fault_no_display." fk "." dev; cls = 2 }
  else if (hasf && !hasp) { g = "fault_static." fk "." dev; cls = 3 }
  else if (hasf) { g = "fault_alive." fk "." dev; cls = 4 }
  else if (!hasd) {
    cls = 5; sp = (fnb == "none") ? 3 : ((fnb + 0 <= 600) ? 1 : 2)
    g = "dark_no_fault." (sp == 3 ? "never_draws" : (sp == 1 ? "latch_miss" : "late"))
  } else if (!hasp) {
    cls = 6; sp = (d18 + 0 >= 2) ? 1 : ((ram == "still") ? 2 : 3)
    g = "static_no_fault." (sp == 1 ? "animates_late" : (sp == 2 ? "frozen" : "alive_still"))
  }
  if (g == "") next
  groups[g]++; grouped++
  leafkey[g] = cls " " sp " " ((cls >= 2 && cls <= 4) ? idx(fk, kinds) " " idx(dev, devs) : "0 0")
  if (ps != "none") subs[g SUBSEP 1 SUBSEP ps]++
  if (wh != "none") subs[g SUBSEP 2 SUBSEP wh]++
  if (pc != "none") subs[g SUBSEP 3 SUBSEP pc]++
  # The class sub-rows join their parts with a colon: a dot before a machine name would read as a file name to the leak scan.
  if (fa != "none") subs[g SUBSEP 4 SUBSEP (fa ":" p2)]++
  if (vm != "none") subs[g SUBSEP 5 SUBSEP (vm ":" $3 ":" mp)]++
  if (wc != "none") subs[g SUBSEP 6 SUBSEP (wc ":" mp)]++
  if (pcl != "none") subs[g SUBSEP 7 SUBSEP (pcl ":" $3 ":" $4)]++
  subs[g SUBSEP 8 SUBSEP (sz ":" hd ":" mp)]++
  mc[mp SUBSEP cls]++; sl[src SUBSEP g]++
  t = title[$2]
  if (length(t) > 80 || t !~ tre) { t = "-"; withheld++ }
  printf "D diag: title=%s group=%s first_display=%s first_nonblank=%s distinct600=%s distinct1800=%s fault=%s dev=%s pc=%s mapper=%s first_change=%s fault_frame=%s order=%s where=%s ram=%s source=%s\n", t, g, fd, fnb, d6, d18, fk, dev, pc, mp, fc, ff, ord, wh, ram, src
  next
}
{ bad++ }
END {
  # Leaf lines and sub-rows carry numeric sort keys (rows, class, split, kind, device, line kind, sub-row key) that sh removes after sorting.
  for (g in groups) printf "G %d %s 0 0 diag-group: leaf=%s rows=%d\n", groups[g], leafkey[g], g, groups[g]
  if (!("input_mismatch" in groups)) print "G 0 1 0 0 0 0 0 diag-group: leaf=input_mismatch rows=0"
  split("pcslot where pc faddr vdpmode wclass pclass size", label, " ")
  for (k in subs) {
    split(k, a, SUBSEP)
    # An exact pc is shown only when 5 or more rows share it.
    if (a[2] == 3 && subs[k] < 5) continue
    printf "G %d %s %d %s diag-group: leaf=%s %s=%s rows=%d\n", groups[a[1]], leafkey[a[1]], a[2], (a[2] == 2 ? idx(a[3], wheres) : a[3]), a[1], label[a[2]], a[3], subs[k]
  }
  # Cross-cuts and the pass histogram: counts only, tagged with sort keys (mapper, class, source, leaf keys, first frame of the bucket).
  for (k in mc) { split(k, a, SUBSEP); printf "X %s %d diag-other: mapper=%s class=%s rows=%d\n", a[1], a[2], a[1], cn[a[2]], mc[k] }
  for (k in sl) { split(k, a, SUBSEP); printf "S %s %s diag-other: source=%s leaf=%s rows=%d\n", a[1], leafkey[a[2]], a[1], a[2], sl[k] }
  for (k in hist) {
    split(k, a, SUBSEP)
    printf "H %s %d diag-other: pass mapper=%s first_change=%s rows=%d\n", a[1], a[2], a[1], (a[2] == 999999 ? "none" : (a[2] "-" (a[2] + 59))), hist[k]
  }
  printf "O diag-other: candidates=%d attempted=%d evaluated=%d malformed=%d malformed_io=%d uncatalogued=%d skipped_excluded=%d noprofile=%d unsupported=%d infrastructure_error=%d title_withheld=%d\n", candidates, attempted, evaluated, bad, mio, unc, excl, nop, unsup, infra, withheld
  # Ledger fail rows of the default profile, keyed like a record: id, machine, region, tv and fm.
  for (n = 1; n <= ln; n++) {
    if (L[n, "status"] != "fail" || L[n, "parameter"] != "boots@600" || L[n, "port1"] != "pad" || L[n, "port2"] != "pad" ||
        L[n, "script"] != "none" || L[n, "tv"] != "ntsc" || !hex40(L[n, "id"])) continue
    lf++; key = L[n, "id"] SUBSEP L[n, "machine"] SUBSEP L[n, "region"] SUBSEP L[n, "tv"] SUBSEP L[n, "fm"]; lfk[key] = 1
    if (key in runfail) matched++
    else if (key in runpass) nowpass++
    else notin++
  }
  for (i = 1; i <= failed; i++) if (!(rf[i] in lfk)) rnl++
  printf "O diag-other: judged=%d run_fail=%d ledger_fail=%d matched=%d ledger_fail_not_in_run=%d ledger_fail_now_pass=%d run_fail_not_in_ledger=%d groups_total=%d fail_total=%d\n", evaluated, failed, lf, matched, notin, nowpass, rnl, grouped, failed
  exit grouped != failed
}' "$COGMASTER_CATALOG" "$tmp/cases" "$tmp/records" > "$tmp/lines" 2>/dev/null; then rc=0; else rc=$?; fi
  [ "$rc" -le 1 ] || { echo 'library: diagnose could not read its records' >&2; exit 1; }
  {
    printf 'library-diagnose 1 core=%s behaviour=%s\n' "$core" "$behaviour"
    sed -n 's/^D //p' "$tmp/lines" | sort
    sed -n 's/^G //p' "$tmp/lines" | sort -k1,1nr -k2,2n -k3,3n -k4,4n -k5,5n -k6,6n -k7,7 | cut -d' ' -f8-
    sed -n 's/^X //p' "$tmp/lines" | sort -k1,1 -k2,2n | cut -d' ' -f3-
    sed -n 's/^S //p' "$tmp/lines" | sort -k1,1 -k2,2n -k3,3n -k4,4n -k5,5n | cut -d' ' -f6-
    sed -n 's/^H //p' "$tmp/lines" | sort -k1,1 -k2,2n | cut -d' ' -f3-
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
