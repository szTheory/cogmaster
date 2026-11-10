#!/bin/sh
# Formats the weekly CI digest, and reads job timeouts out of workflow files.
#   sh scripts/digest.sh < input.json              Markdown on stdout
#   sh scripts/digest.sh timeouts <workflow file>...   JSON object, display name to minutes
# Fault prevented: a digest that misses a job whose timeout margin is spent
# because it joined on the YAML key (ci-required) where ci-durations prints
# the display name (CI required). Both sides use the display name: the
# job-level name with a trailing " (...)" matrix suffix removed, else the key.
# Input keys: runs, jobs, failed_attempts (earlier attempts of runs that later
# succeeded), timeouts (from the timeouts mode), nightly_issues and pulls.
# It only formats what it is given; it never reads the remote.
set -eu
LC_ALL=C
export LC_ALL
here=$(dirname "$0")

if [ "${1-}" = timeouts ]; then
  shift
  awk '
    function flush() { if (key != "" && t != "") print (name != "" ? name : key) "\t" t }
    FNR == 1 { flush(); key = ""; injobs = 0 }
    /^jobs:/ { injobs = 1; next }
    /^[^ #]/ { flush(); key = ""; injobs = 0 }
    injobs && /^  [A-Za-z0-9_-]+:[ \t]*$/ { flush(); key = $1; sub(/:$/, "", key); name = ""; t = ""; next }
    injobs && /^    name:/ { name = $0; sub(/^    name:[ \t]*/, "", name); gsub(/^["\047]|["\047]$/, "", name); sub(/ \(.*\)$/, "", name) }
    injobs && /^    timeout-minutes:/ { t = $2 + 0 }
    END { flush() }
  ' "$@" | jq -R -s 'split("\n") | map(select(. != "") | split("\t") | { key: .[0], value: (.[1] | tonumber) }) | from_entries'
  exit
fi

# Every answer goes to a file before jq reads it: a reader that quits early
# would otherwise close the pipe under the writer.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cat > "$work/in.json"
jq '.jobs' "$work/in.json" > "$work/jobs.json"
sh "$here/ci-durations.sh" summarise < "$work/jobs.json" > "$work/slowest.tsv"

jq -r --rawfile slowest "$work/slowest.tsv" '
  def tenth: . * 10 | round | . / 10;
  def secs: (.completed_at | fromdateiso8601) - (.started_at | fromdateiso8601);
  ($slowest | split("\n") | .[1:] | map(select(. != "") | split("\t"))) as $rows
  | (.runs // []) as $runs | (.jobs // []) as $jobs | (.failed_attempts // []) as $earlier
  | (.timeouts // {}) as $limits | (.nightly_issues // []) as $issues | (.pulls // []) as $pulls
  | [ $issues[] | "- nightly issue #\(.number): \(.title)" ] as $open
  | [ $pulls[] | select(.red == true or (now - (.updated_at | fromdateiso8601)) > 604800)
      | "- pull request #\(.number) is \(if .red == true then "red" else "idle for seven days" end): \(.title)" ] as $stuck
  | [ $rows[] | select($limits[.[0]] != null and (.[2] | tonumber) * 2 > $limits[.[0]] * 60)
      | "- timeout margin: \(.[0]), \(.[2]) s slowest against a \($limits[.[0]]) min timeout, run \(.[3])" ] as $spent
  | [ $runs[] | select(.run_attempt > 1 and .conclusion == "success") as $r
      | $earlier[] | select(.run_id == $r.id and .conclusion == "failure")
      | "Flaky: \(.name) failed an earlier attempt of run \($r.id) (\($r.html_url))" ] as $flaky
  | "## CI digest", "",
    "### CI duration",
    ($runs | group_by(.workflow)[] | map(.id) as $ids
      | "- \(.[0].workflow): \(length) run\(if length == 1 then "" else "s" end),\([$jobs[] | select(.started_at != null and .completed_at != null and (.run_id | IN($ids[]))) | secs] | add // 0 | . / 60 | tenth) job minutes"),
    "",
    (if ($rows | length) > 0 then "Slowest job: \($rows[0][0]), \($rows[0][2]) s, run \($rows[0][3])" else "Slowest job: none" end),
    (if ($flaky | length) > 0 then $flaky[] else "Flaky: none" end),
    "",
    "### Open nightly issues",
    (if ($open | length) > 0 then $open[] else "- none" end),
    "",
    "### Attention",
    (($open + $stuck + $spent) as $items | if ($items | length) > 0 then $items[] else "- none" end),
    "",
    "attention: \(($open + $stuck + $spent) | length)"
' "$work/in.json"
