#!/bin/sh
# Explicit public update only; stdout is a candidate ledger, never private diagnostics.
set -eu; top=$(git rev-parse --show-toplevel)
COMPAT_REV=$(git rev-parse HEAD) COMPAT_CATALOG=${COMPAT_CATALOG:-$top/tests/compat/catalog.tsv} COMPAT_REASONS=${COMPAT_REASONS:-$top/tests/compat/reasons.txt} sh "$(dirname "$0")/compat-common.sh" update "${COMPAT_LEDGER:-$top/tests/compat/ledger.csv}" "$@"
