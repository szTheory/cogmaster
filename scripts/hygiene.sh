#!/bin/sh
# Faults prevented: private data, unlisted images/binaries, GPL code, unsafe GSD settings and commit identities.
# Kept POSIX so both the commit hook and hosted dash execute the same scanner.
set -eu
LC_ALL=C
export LC_ALL
git() { command git -c core.quotePath=false "$@"; }
cd "$(git rev-parse --show-toplevel)"
SELF=scripts/hygiene.sh
MANIFEST=tests/roms/manifest.txt
fail=0
found() { printf 'hygiene: %s: %s\n' "$1" "$2" >&2; fail=1; }
HOME_PATH='(/Users/|/home/)[A-Za-z0-9._-]+|[A-Za-z]:\\Users\\[A-Za-z0-9._-]+|-Users-[A-Za-z0-9._]+-'
HOME_OK='(/Users/|/home/)runner'
EMAIL='[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)*\.[A-Za-z][A-Za-z]+'
EMAIL_OK='@users\.noreply\.github\.com|noreply@github\.com|noreply@anthropic\.com|@example\.(com|org|net)|git@github\.com'
ROM_NAME='\.(sms|gg|sg|sc|mv|bin|rom|sav|srm|state|eeprom)$'
GPL_TEXT='GNU (Lesser |Library )?General Public License|SPDX-License-Identifier:[[:space:]]*L?GPL'
CODE_PATH='^(src|include|runner|libretro|host|tests|tools|cmake)/|CMakeLists\.txt$'
export HOME_PATH HOME_OK EMAIL EMAIL_OK GPL_TEXT CODE_PATH
in_manifest() { [ -f "$MANIFEST" ] && awk -v p="$1" '$1==p {f=1} END {exit !f}' "$MANIFEST"; }
check_text() {
  awk -v path="$1" -v code="${2-}" '
  function found(rule) {print "hygiene: " rule ": " path > "/dev/stderr"; bad=1}
  $0 ~ ENVIRON["HOME_PATH"] && $0 !~ ENVIRON["HOME_OK"] {found("home directory path")}
  $0 ~ ENVIRON["EMAIL"] && $0 !~ ENVIRON["EMAIL_OK"] {found("email address")}
  code ~ ENVIRON["CODE_PATH"] && $0 ~ ENVIRON["GPL_TEXT"] {found("GPL licence text in code")}
  END {exit bad}'
}
check_file() {
  [ "$1" = "$SELF" ] && return 0
  if ! $2 "$1" >/dev/null 2>&1; then found 'cannot read file' "$1"; return 0; fi
  if printf '%s\n' "$1" | grep -qiE "$ROM_NAME" && ! in_manifest "$1"; then found 'game or BIOS image by name' "$1"; fi
  for off in 32752 16368 8176; do
    got=$($2 "$1" | dd bs=1 skip=$off count=8 2>/dev/null | od -An -tx1 | tr -d ' \n')
    if [ "$got" = 544d522053454741 ] && ! in_manifest "$1"; then found 'game image by content' "$1"; break; fi
  done
  size=$($2 "$1" | head -c 1 | wc -c | tr -d ' ')
  if [ "$size" != 0 ] && ! $2 "$1" | grep -Iq ''; then in_manifest "$1" || found 'binary file not in manifest' "$1"
  else $2 "$1" | check_text "$1" "$1"; fi
}
check_files() {
  old_ifs=$IFS; IFS='
'
  set -f
  for f in $1; do check_file "$f" "$2" || fail=1; done
  set +f
  IFS=$old_ifs
}
check_stop_policy() {
  [ "${COGMASTER_ALLOW_CHAIN:-0}" = 1 ] && return 0
  [ -f .planning/config.json ] || return 0
  grep -Eq '"mode"[[:space:]]*:[[:space:]]*"interactive"' .planning/config.json || found 'GSD mode is not interactive' .planning/config.json
  if grep -Eq '"(auto_advance|_auto_chain_active)"[[:space:]]*:[[:space:]]*true' .planning/config.json; then found 'GSD auto-advance or chain flag is on' .planning/config.json; fi
}
check_identity() {
  case $1 in *@users.noreply.github.com | noreply@github.com) ;; *) found 'commit identity is not a noreply address' "$2" ;; esac
}
staged_show() { git show ":$1"; }
tree_show() { if [ -f "$1" ]; then cat "$1"; else git show ":$1"; fi; }
mode=${1:---tree}
[ $# -gt 0 ] && shift
case $mode in
  --staged)
    check_files "$(git diff --cached --name-only -z --diff-filter=ACMR | tr '\0' '\n')" staged_show
    check_stop_policy
    if [ "${COGMASTER_ALLOW_MAIN_COMMIT:-0}" != 1 ] && [ -n "$(git remote)" ] && [ "$(git symbolic-ref --quiet --short HEAD || true)" = main ]; then found 'commit on main once a remote exists; branch first' 'git switch -c phase/NN-slug origin/main'; fi
    check_identity "$(git var GIT_AUTHOR_IDENT | sed -E 's/.*<([^>]*)>.*/\1/')" author
    check_identity "$(git var GIT_COMMITTER_IDENT | sed -E 's/.*<([^>]*)>.*/\1/')" committer ;;
  --tree)
    check_files "$(git ls-files -z --cached --others --exclude-standard | tr '\0' '\n')" tree_show
    check_stop_policy ;;
  --history)
    [ $# -gt 0 ] || set -- --all
    for c in $(git rev-list "$@"); do
      git grep -I -h -e '' "$c" -- . ":!$SELF" | check_text "commit $c" || fail=1
      git log -1 --format=%B "$c" | check_text "message of $c" || fail=1
      for b in $(git diff-tree -r --root --no-commit-id --numstat "$c" | awk -F '\t' '$1=="-" && $2=="-" {print $3}'); do
        [ "$b" = "$SELF" ] || in_manifest "$b" || found 'binary file not in manifest' "$b (in history)"
      done
    done
    for e in $(git log "$@" --format='%ae%n%ce' | sort -u); do check_identity "$e" history; done
    for n in $(git log "$@" --name-only --format= | sort -u | grep -iE "$ROM_NAME" || true); do in_manifest "$n" || found 'game or BIOS image by name' "$n (in history)"; done ;;
  *) echo "usage: $0 --staged | --tree | --history [revs...]" >&2; exit 2 ;;
esac
exit "$fail"
