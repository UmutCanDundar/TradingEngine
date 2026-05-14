#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/benchmark/order_manager/order_manager_bench"

echo -e "\nRunning order_manager bench test..."

cd $ROOT
"$TEST_BIN"
