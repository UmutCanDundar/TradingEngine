#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/perf/order_manager/order_manager_perf"

BLUE='\033[0;34m'
NC='\033[0m'

echo -e "\n${BLUE}Running order_manager perf test...${NC}"

cd $ROOT
"$TEST_BIN"
