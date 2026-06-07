#!/usr/bin/env bash
set -e 

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

RED='\033[0;31m'
NC='\033[0m'

echo -e "\n\n${RED}RUNNING ALL BENCH TESTS...${NC}"

"$SOURCE/benchmark/run_builder_bench.sh" || true
"$SOURCE/benchmark/run_parser_bench.sh" || true
"$SOURCE/benchmark/run_order_manager_bench.sh" || true
"$SOURCE/benchmark/run_risk_bench.sh" || true  

