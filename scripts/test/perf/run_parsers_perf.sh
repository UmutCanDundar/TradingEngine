#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/perf/parser/parser_dispatch_perf"

echo -e "\nRunning parsers perf test..."

cd $ROOT
"$TEST_BIN"
