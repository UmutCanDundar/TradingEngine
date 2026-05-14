#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/benchmark/risk/risk_bench"

echo -e "\nRunning risk bench test..."

cd $ROOT
"$TEST_BIN"
