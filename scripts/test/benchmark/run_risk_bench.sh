#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/benchmark/risk/risk_bench"

RED='\033[0;31m'
NC='\033[0m'

echo -e "\n${RED}Running risk bench test...${NC}"

cd $ROOT
"$TEST_BIN"
