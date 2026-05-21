#!/usr/bin/env bash
set -e
SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$SOURCE/run_all_unit_test.sh"
"$SOURCE/run_all_bench_test.sh"
"$SOURCE/run_all_perf_test.sh"
"$SOURCE/run_all_pipeline_test.sh"
