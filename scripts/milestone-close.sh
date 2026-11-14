#!/bin/sh
# Checks that the repository is a clean copy of main before a milestone-close pull request.
#   sh scripts/milestone-close.sh     from a checkout of main, with gh signed in
# It prints one row per check, "name  ok|off|unknown  reason", then one summary line: open-prs (any open pull
# request), main-ci (CI required on the head of main), release (latest release against the manifest, no draft,
# CI required on its tag), release-pr (a merged release pull request still pending), tree (on main, clean,
# at the remote head) and unreleased (feat, fix or perf commits since the latest tag). The reasons hold counts
# and 7-character shas only. Exit status: 0 when every row is ok, 1 when any row is off, 2 when no row is off
# but one could not be read (a failed gh call, a rate limit, a CI run still in progress).
# Fault prevented: a milestone closed while a pull request is open, CI or a release is red or stuck, a
# releasable commit is unreleased, or the work tree is not a clean copy of main. It only reads: no write
# request, no retry. Every gh answer is taken as v=$(gh ...) || v=unknown and never goes through a pipe,
# so a failed call is an unknown row and cannot be a green one.
set -eu
LC_ALL=C
export LC_ALL

n_off=0
n_unknown=0
row() {
  case $2 in off) n_off=$((n_off + 1)) ;; unknown) n_unknown=$((n_unknown + 1)) ;; esac
  printf '%-11s  %s  %s\n' "$1" "$2" "$3"
}
short() { printf '%s' "$1" | cut -c1-7; }
# The word of a CI required check run: success is ok, a failed end is off, anything else is not yet known.
verdict() {
  case $1 in success) echo ok ;; failure | cancelled | timed_out) echo off ;; *) echo unknown ;; esac
}
# A count row: 0 is ok, a positive number is off, anything else is unknown.
count() {
  case $2 in
    0) row "$1" ok "$3" ;;
    '' | *[!0-9]*) row "$1" unknown "not readable" ;;
    *) row "$1" off "$2 $4" ;;
  esac
}

repo=$(gh repo view --json nameWithOwner --jq .nameWithOwner) || repo=unknown
main=$(gh api "repos/$repo/commits/main" --jq .sha) || main=unknown
branch=$(git symbolic-ref --short -q HEAD) || branch=detached
dirty=$(git status --porcelain)
head=$(git rev-parse HEAD)

n=$(gh api "repos/$repo/pulls?state=open&per_page=100" --jq length) || n=unknown
count open-prs "$n" "0 open" "open"

if [ "$main" = unknown ]; then
  row main-ci unknown "remote main unknown"
else
  w=$(gh api "repos/$repo/commits/$main/check-runs?check_name=CI%20required" --jq '.check_runs[0] | .conclusion // .status') || w=unknown
  row main-ci "$(verdict "$w")" "CI required $w at $(short "$main")"
fi

m=$(git show origin/main:.release-please-manifest.json) || m=unknown
ver=
[ "$m" = unknown ] || ver=$(printf '%s\n' "$m" | sed -n 's/.*"\.":[[:space:]]*"\([^"]*\)".*/\1/p')
want=unknown
[ -z "$ver" ] || want=v$ver
latest=$(gh api "repos/$repo/releases/latest" --jq .tag_name) || latest=unknown
drafts=$(gh api --paginate "repos/$repo/releases" --jq '.[] | select(.draft) | .id') || drafts=unknown
tagci=unknown
[ "$latest" = unknown ] || tagci=$(gh api "repos/$repo/commits/$latest/check-runs?check_name=CI%20required" --jq '.check_runs[0] | .conclusion // .status') || tagci=unknown
if [ "$latest" != unknown ] && [ "$want" != unknown ] && [ "$latest" != "$want" ]; then
  row release off "latest $latest, manifest $want"
elif [ "$drafts" != unknown ] && [ -n "$drafts" ]; then
  row release off "a draft release exists"
elif [ "$(verdict "$tagci")" = off ]; then
  row release off "CI required $tagci on $latest"
elif [ "$latest" = unknown ] || [ "$want" = unknown ] || [ "$drafts" = unknown ] || [ "$(verdict "$tagci")" != ok ]; then
  row release unknown "latest $latest, manifest $want, drafts ${drafts:-none}, tag CI $tagci"
else
  row release ok "$latest, no draft, CI success"
fi

p=$(gh api "search/issues?q=repo:$repo+is:pr+is:merged+label:%22autorelease:%20pending%22" --jq .total_count) || p=unknown
count release-pr "$p" "none pending" "merged still pending: the tag step failed"

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

u=unknown
[ "$latest" = unknown ] || u=$(gh api "repos/$repo/compare/$latest...main" --jq '[.commits[].commit.message | split("\n")[0] | select(test("^(feat|fix|perf)[(!:]"))] | length') || u=unknown
count unreleased "$u" "none since $latest" "releasable commits since $latest"

if [ $((n_off + n_unknown)) -eq 0 ]; then
  echo 'close-check: all ok'
  exit 0
fi
echo "close-check: $n_off off, $n_unknown unknown"
if [ "$n_off" -gt 0 ]; then
  exit 1
fi
exit 2
