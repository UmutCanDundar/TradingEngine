#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

MODE=${1:-all}

if [[ "$MODE" == "all" ]]; then
    bash "$SCRIPT_DIR/scripts/test/run_all_test.sh"
elif [[ "$MODE" == "pipeline" ]]; then
    bash "$SCRIPT_DIR/scripts/test/run_all_pipeline_test.sh"
elif [[ "$MODE" == "bench" ]]; then
    bash "$SCRIPT_DIR/scripts/test/run_all_bench_test.sh"
elif [[ "$MODE" == "perf" ]]; then
    bash "$SCRIPT_DIR/scripts/test/run_all_perf_test.sh"
elif [[ "$MODE" == "unit" ]]; then
    bash "$SCRIPT_DIR/scripts/test/run_all_unit_test.sh"
else
    echo "Unknown mode: $MODE"
    echo "Usage: $0 [mode]"
    echo "  Modes:"
    echo "    all           Run all tests (default)"
    echo "    pipeline      Run pipeline(Rx-Tx) tests only"
    echo "    bench         Run benchmark tests only"
    echo "    perf          Run performance tests only"
    echo "    unit          Run unit tests only"
    exit 1
fi

