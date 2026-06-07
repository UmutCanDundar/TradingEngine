#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
TEST_BIN="$ROOT/build/test/benchmark/order_manager/ITCH/order_manager_itch_nasdaq_single_bench"
RESULT_DIR="$ROOT/test/_results/benchmark/order_manager"
RESULTS="$RESULT_DIR/order_manager_itch_nasdaq_single_results.txt"

mkdir -p "$RESULT_DIR"

RED='\033[0;31m'
NC='\033[0m'

echo -e "\n${RED}Running order_manager_itch_nasdaq_single bench test...${NC}"

cd $ROOT
"$TEST_BIN" \
    --benchmark_min_time=2s \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=true \
    --benchmark_counters_tabular=true \
    2>&1 | tee -a "$RESULTS"
