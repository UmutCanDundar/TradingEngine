#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/order_manager/order_manager_unit"

YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "\n${YELLOW}Running order_manager unit test...${NC}"

cd $ROOT
"$TEST_BIN"
