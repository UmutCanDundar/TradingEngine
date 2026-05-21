#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/benchmark/builder/builder_dispatch_bench"

RED='\033[0;31m'
NC='\033[0m'

echo -e "\n${RED}Running builders bench test...${NC}"

cd $ROOT
"$TEST_BIN"
