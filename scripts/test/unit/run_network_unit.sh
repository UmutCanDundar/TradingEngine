#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/network/network_unit"
RESULT_DIR="$ROOT/test/_results/unit"
RESULTS="$RESULT_DIR/network_results.txt"

mkdir -p "$RESULT_DIR"

YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "\n${YELLOW}Running network unit test...${NC}"

cd $ROOT
"$TEST_BIN" 2>&1 | tee -a "$RESULTS"
