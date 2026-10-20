#!/bin/sh
# Fault prevented: a candidate replaces the judge or silently rewrites passing compatibility facts.
set -eu
[ "$#" -ge 6 ] && [ "$#" -le 7 ] && [ -f "$3" ] && [ "$(wc -c < "$3")" -le 4194304 ] || exit 2
COMPAT_ALLOW=$3 COMPAT_TODAY=$4 COMPAT_BASE_REV=$5 COMPAT_HEAD_REV=$6
export COMPAT_ALLOW COMPAT_TODAY COMPAT_BASE_REV COMPAT_HEAD_REV
if [ "$#" = 7 ]; then set -- "$1" "$2" "$7"; else set -- "$1" "$2"; fi
exec sh "$(dirname "$0")/compat-common.sh" ratchet "$@"
