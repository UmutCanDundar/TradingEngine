#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/risk/risk_unit"

echo -e "\nRunning risk unit test..."

cd $ROOT
"$TEST_BIN"