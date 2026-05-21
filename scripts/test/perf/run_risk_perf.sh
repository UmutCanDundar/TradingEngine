#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN1="$ROOT/build/test/perf/risk/risk_update_perf"
TEST_BIN2="$ROOT/build/test/perf/risk/risk_check_perf"

BLUE='\033[0;34m'
NC='\033[0m'

echo -e "\n${BLUE}Running risk perf test...${NC}"

cd $ROOT
"$TEST_BIN1"
"$TEST_BIN2"
