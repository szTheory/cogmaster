#!/bin/sh
# Explicit public update only; stdout is a candidate ledger, never private diagnostics.
set -eu
COMPAT_CATALOG="$(git rev-parse --show-toplevel)/src/cartdb.inc" COMPAT_REV="$(git rev-parse HEAD)" sh "$(dirname "$0")/compat-common.sh" update "$(git rev-parse --show-toplevel)/tests/compat/ledger.csv" "$@"
