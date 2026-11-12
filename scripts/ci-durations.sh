#!/bin/sh
# Reports how long each CI job really took, so a timeout is twice a measured
# run and names that run.
#   sh scripts/ci-durations.sh <workflow file name> [count]   live report
#   sh scripts/ci-durations.sh summarise                      JSON array on stdin
#   sh scripts/ci-durations.sh tests <run id>                 slowest seconds per test
#   sh scripts/ci-durations.sh tests-summarise                ctest result lines on stdin
# Fault prevented: a timeout guessed, or sized from a cancelled or failed run
# or from one local run, and a report that counts a failed test as a time.
# The live mode lists the workflow's last count successful runs (default 40,
# at most 100) on any branch and event, so no job has more than count runs
# behind it, and reads each run's jobs. It only reads; it never changes the
# repository or the remote.
# summarise reads [{run_id, name, conclusion, started_at, completed_at}] and
# keeps successful jobs only. A matrix suffix " (...)" is stripped to give the
# job's display name, and a name holding " / " is a reusable workflow's nested
# job, measured from its own workflow, so it is skipped. N counts distinct
# runs, so lanes of one run count once; the slowest seconds and their run
# come from the slowest lane instance. Output is tab-separated, slowest job
# first: job, runs, slowest seconds, run ID, minutes (twice the slowest
# rounded up, at least 1) and the comment text for the timeout line.
# tests-summarise reads ctest result lines in any prefix; only a Passed line,
# "Test #N: name ..... Passed S sec", counts. It prints test, lanes that passed
# it and slowest seconds, tab-separated, slowest first; tests reads a run's log.
set -eu
LC_ALL=C
export LC_ALL

summarise() {
  jq -r '
    [ .[] | select(.conclusion == "success" and .started_at != null and .completed_at != null)
      | select(.name | contains(" / ") | not)
      | { id: .run_id, name: (.name | sub(" \\(.*\\)$"; "")),
          s: ((.completed_at | fromdateiso8601) - (.started_at | fromdateiso8601)) } ]
    | group_by(.name)
    | map({ name: .[0].name, n: (map(.id) | unique | length), top: max_by(.s) })
    | sort_by(-.top.s)
    | (["job", "runs", "slowest_s", "run_id", "minutes", "comment"] | @tsv),
      (.[] | [ .name, .n, .top.s, .top.id, ([1, (2 * .top.s / 60 | ceil)] | max),
               "2 x \(.top.s) s, slowest of \(.n) run\(if .n == 1 then "" else "s" end), run \(.top.id)" ] | @tsv)'
}

tests_summarise() {
  printf 'test\tlanes\tslowest_s\n'
  awk '
    match($0, /Test +#[0-9]+: +[^ ]+ +\.+ +Passed +[0-9.]+ sec/) {
      n = split(substr($0, RSTART, RLENGTH), f, / +/)
      lanes[f[3]]++
      if (lanes[f[3]] == 1 || f[n - 1] + 0 > top[f[3]] + 0) top[f[3]] = f[n - 1]
    }
    END { for (t in lanes) printf "%s\t%d\t%s\n", t, lanes[t], top[t] }' | sort -t "$(printf '\t')" -k3,3nr -k1,1
}

case ${1-} in
  summarise) summarise; exit ;;
  tests-summarise) tests_summarise; exit ;;
  tests)
    case ${2-} in '' | *[!0-9]*) echo 'usage: ci-durations.sh tests <run id>' >&2; exit 2 ;; esac
    log=$(mktemp)
    trap 'rm -f "$log"' EXIT
    gh run view "$2" --log > "$log"
    tests_summarise < "$log"
    exit ;;
esac

workflow=${1-}
count=${2-40}
case $workflow in '' | */* | -*) echo 'usage: ci-durations.sh <workflow file name> [count] | summarise | tests <run id>' >&2; exit 2 ;; esac
case $count in '' | *[!0-9]*) echo 'ci-durations: count must be a number from 1 to 100' >&2; exit 2 ;; esac
[ "$count" -ge 1 ] && [ "$count" -le 100 ] || { echo 'ci-durations: count must be a number from 1 to 100' >&2; exit 2; }

repo=$(gh repo view --json nameWithOwner --jq .nameWithOwner)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
# Every answer goes to a file before jq reads it: a reader that quits early
# would otherwise close the pipe under the writer.
gh api "repos/$repo/actions/workflows/$workflow/runs?status=success&per_page=$count" > "$work/runs.json"
jq -r --argjson count "$count" '.workflow_runs[:$count][].id' "$work/runs.json" > "$work/ids.txt"
printf '{"jobs":[]}\n' > "$work/jobs-none.json"
while IFS= read -r id; do
  gh api "repos/$repo/actions/runs/$id/jobs?per_page=100" > "$work/jobs-$id.json"
done < "$work/ids.txt"
jq -s '[.[].jobs[] | {run_id, name, conclusion, started_at, completed_at}]' "$work"/jobs-*.json > "$work/all.json"
summarise < "$work/all.json"
