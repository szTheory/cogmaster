#!/bin/sh
# Repository hygiene for a public repo: no personal data, no game or BIOS
# images, no binary file the fixture manifest does not list, no GPL text in
# code, no chain-mode GSD settings, and no commits on main once a remote exists.
#
#   scripts/hygiene.sh --staged             the commit being made       (pre-commit)
#   scripts/hygiene.sh --tree               every file git would track  (CI)
#   scripts/hygiene.sh --history [revs...]  commits; default --all      (pre-push)
#
# Findings name the rule and the file, never the matched text, so the output is
# safe to paste anywhere. Exit status is 1 if anything was found.
# Adapted from the owner's nesturbator project (MIT).
set -eu

# The C locale for every command: under a UTF-8 locale grep -I treats bytes that
# are not valid UTF-8 as binary and skips the file. In the C locale only a NUL
# byte makes a file binary.
LC_ALL=C
export LC_ALL

# Path quoting off, so a name with bytes above 0x7F is scanned, not skipped.
git() { command git -c core.quotePath=false "$@"; }

cd "$(git rev-parse --show-toplevel)"

SELF=scripts/hygiene.sh
MANIFEST=tests/roms/manifest.txt
fail=0
found() { printf 'hygiene: %s: %s\n' "$1" "$2" >&2; fail=1; }

# A home directory names a person (also in the dashed form agent tools use for
# project folders). Hosted CI runner homes are the exception.
HOME_PATH='(/Users/|/home/)[A-Za-z0-9._-]+|[A-Za-z]:\\Users\\[A-Za-z0-9._-]+|-Users-[A-Za-z0-9._]+-'
HOME_OK='(/Users/|/home/)runner'
# Any address except GitHub noreply, the AI co-author noreply, documentation
# domains and SSH remotes.
EMAIL='[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)*\.[A-Za-z][A-Za-z]+'
EMAIL_OK='@users\.noreply\.github\.com|noreply@github\.com|noreply@anthropic\.com|@example\.(com|org|net)|git@github\.com'
# Master System, Game Gear, SG-1000 and SC-3000 images, BIOS dumps, saves and states.
ROM_NAME='\.(sms|gg|sg|sc|mv|bin|rom|sav|srm|state|eeprom)$'
# GPL or LGPL licence text must never appear in code (PROVENANCE.md).
GPL_TEXT='GNU (Lesser |Library )?General Public License|SPDX-License-Identifier:[[:space:]]*L?GPL'
CODE_PATH='^(src|include|runner|libretro|host|tests|tools|cmake)/|CMakeLists\.txt$'

# stdin is text. True if some line matches $1 and does not match $2.
leaks() { grep -I -E -e "$1" | grep -v -E -e "$2" | grep -q .; }

# True if $1 is the first field of a line in the fixture manifest.
in_manifest() { [ -f "$MANIFEST" ] && awk -v p="$1" '$1 == p { f = 1 } END { exit !f }' "$MANIFEST"; }

# $1 is a path; $2 is a command that prints that path's content. A file whose
# content cannot be read is a finding, so the scan fails closed.
check_file() {
  [ "$1" = "$SELF" ] && return 0
  if ! $2 "$1" >/dev/null 2>&1; then
    found 'cannot read file' "$1"
    return 0
  fi
  if printf '%s\n' "$1" | grep -q -i -E -e "$ROM_NAME" && ! in_manifest "$1"; then
    found 'game or BIOS image by name' "$1"
  fi
  # A Sega export cartridge carries the 8-byte signature "TMR SEGA" at $7FF0
  # (or $3FF0 / $1FF0 on small images), whatever the file is called. The bytes
  # are compared as hex so this script never contains the signature itself.
  magic=$(printf '544d522053454741' )
  for off in 32752 16368 8176; do
    got=$($2 "$1" 2>/dev/null | dd bs=1 skip=$off count=8 2>/dev/null | od -An -tx1 | tr -d ' \n')
    if [ "$got" = "$magic" ] && ! in_manifest "$1"; then
      found 'game image by content' "$1"
      break
    fi
  done
  # A file with a NUL byte is binary and must be listed in the manifest. This
  # also stops BIOS dumps, headerless images, save states and screenshots.
  size=$($2 "$1" 2>/dev/null | head -c 1 | wc -c | tr -d ' ')
  if [ "$size" != 0 ] && ! $2 "$1" 2>/dev/null | grep -I -q '' && ! in_manifest "$1"; then
    found 'binary file not in manifest' "$1"
  fi
  if $2 "$1" 2>/dev/null | leaks "$HOME_PATH" "$HOME_OK"; then found 'home directory path' "$1"; fi
  if $2 "$1" 2>/dev/null | leaks "$EMAIL" "$EMAIL_OK"; then found 'email address' "$1"; fi
  if printf '%s\n' "$1" | grep -q -E -e "$CODE_PATH" && $2 "$1" 2>/dev/null | grep -q -I -E -e "$GPL_TEXT"; then
    found 'GPL licence text in code' "$1"
  fi
}

# $1 is a newline-separated list; $2 as in check_file. A name holding a newline
# splits into parts that name no file, and each part fails closed.
check_files() {
  old_ifs=$IFS
  IFS='
'
  set -f
  for f in $1; do check_file "$f" "$2"; done
  set +f
  IFS=$old_ifs
}

# GSD runs one step per command in this project. The live config governs, so
# the working-tree file is checked. COGMASTER_ALLOW_CHAIN=1 is a bypass for the
# owner's own shell, never set by an agent (AGENTS rule 9).
check_stop_policy() {
  [ "${COGMASTER_ALLOW_CHAIN:-0}" = 1 ] && return 0
  [ -f .planning/config.json ] || return 0
  grep -q -E '"mode"[[:space:]]*:[[:space:]]*"interactive"' .planning/config.json ||
    found 'GSD mode is not interactive' .planning/config.json
  if grep -q -E '"(auto_advance|_auto_chain_active)"[[:space:]]*:[[:space:]]*true' .planning/config.json; then
    found 'GSD auto-advance or chain flag is on' .planning/config.json
  fi
}

check_identity() { # $1 is an email; $2 says where it came from
  case $1 in
    *@users.noreply.github.com | noreply@github.com) ;;
    *) found 'commit identity is not a noreply address' "$2" ;;
  esac
}

# Once the repository has a remote, every change reaches main by pull request.
# COGMASTER_ALLOW_MAIN_COMMIT=1 bypasses this for a deliberate exception.
check_branch() {
  [ "${COGMASTER_ALLOW_MAIN_COMMIT:-0}" = 1 ] && return 0
  [ -n "$(git remote 2>/dev/null)" ] || return 0
  if [ "$(git symbolic-ref --quiet --short HEAD 2>/dev/null || true)" = main ]; then
    found 'commit on main once a remote exists; branch first' 'git switch -c phase/NN-slug origin/main'
  fi
}

# The two readers are called indirectly, as check_file's $2.
# shellcheck disable=SC2329
staged_show() { git show ":$1"; }
# A tracked file deleted from the working tree is still read from the index.
# shellcheck disable=SC2329
tree_show() { if [ -f "$1" ]; then cat "$1"; else git show ":$1"; fi; }

mode=${1:---tree}
[ $# -gt 0 ] && shift
case $mode in
  --staged)
    check_files "$(git diff --cached --name-only -z --diff-filter=ACMR | tr '\0' '\n')" staged_show
    check_stop_policy
    check_branch
    check_identity "$(git var GIT_AUTHOR_IDENT | sed -E 's/.*<([^>]*)>.*/\1/')" 'author'
    check_identity "$(git var GIT_COMMITTER_IDENT | sed -E 's/.*<([^>]*)>.*/\1/')" 'committer'
    ;;
  --tree)
    check_files "$(git ls-files -z --cached --others --exclude-standard | tr '\0' '\n')" tree_show
    check_stop_policy
    ;;
  --history)
    [ $# -gt 0 ] || set -- --all
    for c in $(git rev-list "$@"); do
      if git grep -I -h -E -e "$HOME_PATH" "$c" -- . ":!$SELF" 2>/dev/null | grep -v -E -e "$HOME_OK" | grep -q .; then
        found 'home directory path' "commit $c"
      fi
      if git grep -I -h -E -e "$EMAIL" "$c" -- . ":!$SELF" 2>/dev/null | grep -v -E -e "$EMAIL_OK" | grep -q .; then
        found 'email address' "commit $c"
      fi
      # A squash merge copies the pull request's title and body into the
      # message, so messages are scanned as well.
      if git log -1 --format=%B "$c" | leaks "$HOME_PATH" "$HOME_OK"; then
        found 'home directory path' "message of $c"
      fi
      if git log -1 --format=%B "$c" | leaks "$EMAIL" "$EMAIL_OK"; then
        found 'email address' "message of $c"
      fi
      # --numstat shows a binary file's line counts as "-".
      for b in $(git diff-tree -r --root --no-commit-id --numstat "$c" |
        awk -F '\t' '$1 == "-" && $2 == "-" { print $3 }'); do
        [ "$b" = "$SELF" ] || in_manifest "$b" || found 'binary file not in manifest' "$b (in history)"
      done
    done
    for e in $(git log "$@" --format='%ae%n%ce' | sort -u); do check_identity "$e" 'history'; done
    for n in $(git log "$@" --name-only --format= | sort -u | grep -i -E -e "$ROM_NAME" || true); do
      in_manifest "$n" || found 'game or BIOS image by name' "$n (in history)"
    done
    ;;
  *)
    printf 'usage: %s --staged | --tree | --history [rev-list args]\n' "$0" >&2
    exit 2
    ;;
esac

exit "$fail"
