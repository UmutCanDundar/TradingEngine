#!/usr/bin/env bash
set -e 

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CYAN='\033[0;36m'
NC='\033[0m'

echo -e "\n\n${CYAN}RUNNING ALL PIPELINE TESTS...${NC}"

"$SOURCE/pipeline/run_engine_Rx_bench.sh"  || true
"$SOURCE/pipeline/run_engine_Tx_bench.sh" || true
"$SOURCE/pipeline/run_engine_RxTx_perf.sh" || true




