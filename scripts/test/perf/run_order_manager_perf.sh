#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/perf/order_manager/order_manager_perf"

echo -e "\nRunning order_manager perf test..."

cd $ROOT
"$TEST_BIN"
