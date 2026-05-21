#!/usr/bin/env bash
set -e

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BLUE='\033[0;34m'
NC='\033[0m'

echo -e "\n\n${BLUE}RUNNING ALL PERF TESTS...${NC}"

"$SOURCE/perf/run_builders_perf.sh"
"$SOURCE/perf/run_parsers_perf.sh"
"$SOURCE/perf/run_order_manager_perf.sh"
"$SOURCE/perf/run_risk_perf.sh"

