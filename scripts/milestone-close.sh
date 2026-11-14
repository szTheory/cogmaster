#!/bin/sh
# Checks that the repository is a clean copy of main before a milestone-close pull request.
#   sh scripts/milestone-close.sh     from a checkout of main, with gh signed in
# It prints one row per check, "name  ok|off|unknown  reason", then one summary line. The reasons hold
# counts and 7-character shas only. Exit status: 0 when every row is ok, 1 when any row is off, 2 when
# no row is off but one could not be read (a failed gh call, a rate limit, a CI run still in progress).
# Fault prevented: a milestone closed while a pull request is open, CI or a release is red or stuck, a
# releasable commit is unreleased, or the work tree is not a clean copy of main. It only reads: no write
# request, no retry. Every gh answer is taken as v=$(gh ...) || v=unknown and never goes through a pipe,
# so a failed call is an unknown row and cannot be a green one.
set -eu
LC_ALL=C
export LC_ALL

off=0
unknown=0
row() {
  case $2 in off) off=$((off + 1)) ;; unknown) unknown=$((unknown + 1)) ;; esac
  printf '%-11s  %s  %s\n' "$1" "$2" "$3"
}
short() { printf '%s' "$1" | cut -c1-7; }

repo=$(gh repo view --json nameWithOwner --jq .nameWithOwner) || repo=unknown
main=$(gh api "repos/$repo/commits/main" --jq .sha) || main=unknown
branch=$(git symbolic-ref --short -q HEAD) || branch=detached
dirty=$(git status --porcelain)
head=$(git rev-parse HEAD)

if [ "$branch" != main ]; then
  row tree off "branch $branch"
elif [ -n "$dirty" ]; then
  row tree off "$(printf '%s\n' "$dirty" | grep -c .) changed or untracked"
elif [ "$main" = unknown ]; then
  row tree unknown "remote main unknown"
elif [ "$head" != "$main" ]; then
  row tree off "head $(short "$head") behind or ahead of main $(short "$main")"
else
  row tree ok "main at $(short "$main")"
fi

if [ $((off + unknown)) -eq 0 ]; then
  echo 'close-check: all ok'
  exit 0
fi
echo "close-check: $off off, $unknown unknown"
if [ "$off" -gt 0 ]; then
  exit 1
fi
exit 2
