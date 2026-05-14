#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/benchmark/parser/parser_dispatch_bench"

echo -e "\nRunning parsers bench test..."

cd $ROOT
"$TEST_BIN"
