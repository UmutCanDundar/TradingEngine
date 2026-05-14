#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/perf/builder/builder_dispatch_perf"

echo -e "\nRunning builders perf test..."

cd $ROOT
"$TEST_BIN"
