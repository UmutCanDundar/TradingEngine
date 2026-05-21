#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/risk/risk_unit"

YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "\n${YELLOW}Running risk unit test...${NC}"

cd $ROOT
"$TEST_BIN"
