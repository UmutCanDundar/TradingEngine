#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/pipeline/benchmark/engine_Rx_bench"
RESULT_DIR="$ROOT/test/_results/benchmark"
RESULTS="$RESULT_DIR/engine_Rx_results.txt"

mkdir -p "$RESULT_DIR"

CYAN='\033[0;36m'
NC='\033[0m'

echo -e "\n${CYAN}Running pipeline Rx bench test...${NC}"

cd $ROOT
"$TEST_BIN" --benchmark_min_time=0.5s \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=true \
    --benchmark_counters_tabular=true \
    2>&1 | tee -a "$RESULTS"
