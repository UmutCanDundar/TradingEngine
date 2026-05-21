#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/pipeline/benchmark/engine_Rx_bench"

CYAN='\033[0;36m'
NC='\033[0m'

echo -e "\n${CYAN}Running pipeline Rx bench test...${NC}"

cd $ROOT
"$TEST_BIN"
