#!/usr/bin/env bash
set -e

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "RUNNING ALL BENCH TESTS..."

"$SOURCE/benchmark/run_builders_bench.sh"
"$SOURCE/benchmark/run_parsers_bench.sh"
"$SOURCE/benchmark/run_order_manager_bench.sh"
"$SOURCE/benchmark/run_risk_bench.sh"

