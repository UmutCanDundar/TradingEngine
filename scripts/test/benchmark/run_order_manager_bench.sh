#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/benchmark/order_manager/order_manager_bench"

RED='\033[0;31m'
NC='\033[0m'

echo -e "\n${RED}Running order_manager bench test...${NC}"

cd $ROOT
"$TEST_BIN"
