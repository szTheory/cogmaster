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
case $physical in "$root"/build/*) ;; *) exit 2 ;; esac
chmod 700 "$out"
umask 077
sh "$root/scripts/compat-common.sh" cases "$root/tests/compat/ledger.csv" > "$out/cases.cmake"
printf 'candidate,status,evaluated,attempted\n' > "$out/results.csv"
count=0
# One flat directory, byte order, regular files only; no recursion or symlink traversal.
for image in "$COGMASTER_ROM_DIR"/* "$COGMASTER_ROM_DIR"/.[!.]* "$COGMASTER_ROM_DIR"/..?*; do
  [ -f "$image" ] && [ ! -L "$image" ] || continue
  case $image in *.[sS][mM][sS]|*.[gG][gG]|*.[bB][iI][nN]|*.[rR][oO][mM]) ;; *) continue ;; esac
  count=$((count+1))
  COGMASTER_IMAGE=$image COGMASTER_CANDIDATE=$count cmake -P "$root/scripts/library.cmake" > "$out/$count.judge.txt" 2>&1 ||
    printf '%s,infrastructure_error,0,0\n' "$count" >> "$out/results.csv"
done
awk -F, 'NR>1 {c++; e+=$3; a+=$4; s[$2]++} END {printf "library: candidates=%d attempted=%d evaluated=%d coverage=%d\n",c,a,e,e; for(k in s) print "library: " k "=" s[k]}' "$out/results.csv" | sort
