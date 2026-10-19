#!/bin/sh
# Decides whether a change needs CI's heavy jobs. Reads the changed paths on
# standard input, one per line, with both the old and the new name of a rename,
# and takes the pull request's changed-file count as its argument.
# Fault prevented: a code change skipping CI as "planning only". Prints
# heavy=false only when there is at least one path, every path is under
# .planning/ and the count is at most 3000; anything else, including empty or
# unreadable input or a missing count, prints heavy=true. The files API lists
# at most 3000 files, in path order, so .planning/ paths alone can fill it.
# It reads all of its input, so the writer never sees a closed pipe.
set -u
paths=0
heavy=false
case ${1-} in
  '' | *[!0-9]*) heavy=true ;;
  *) [ "$1" -le 3000 ] || heavy=true ;;
esac
while IFS= read -r path || [ -n "$path" ]; do
  [ -n "$path" ] || continue
  paths=$((paths + 1))
  case "$path" in
    .planning/*) ;;
    *) heavy=true ;;
  esac
done
[ "$paths" -gt 0 ] || heavy=true
echo "heavy=$heavy"
