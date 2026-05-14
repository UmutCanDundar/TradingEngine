#!/usr/bin/env bash
set -e

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "RUNNING ALL PERF TESTS..."

"$SOURCE/perf/run_builders_perf.sh"
"$SOURCE/perf/run_parsers_perf.sh"
"$SOURCE/perf/run_order_manager_perf.sh"
"$SOURCE/perf/run_risk_perf.sh"

