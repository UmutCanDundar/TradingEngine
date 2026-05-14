#!/usr/bin/env bash
set -e

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "RUNNING ALL TESTS...\n\n"

echo -e "\n\nRUNNING ALL UNIT TESTS...\n\n"
"$SOURCE/run_all_unit_test.sh"

echo -e "\n\nRUNNING ALL BENCH TESTS...\n\n"
"$SOURCE/run_all_bench_test.sh"

echo -e "\n\nRUNNING ALL PERF TESTS...\n\n"
"$SOURCE/perf/run_all_perf_test.sh"



