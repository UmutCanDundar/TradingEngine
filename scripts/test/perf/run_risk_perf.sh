#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN1="$ROOT/build/test/perf/risk/risk_update_perf"
TEST_BIN2="$ROOT/build/test/perf/risk/risk_check_perf"

echo -e "\nRunning risk perf test..."

cd $ROOT
"$TEST_BIN1"
"$TEST_BIN2"
