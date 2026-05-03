#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/order_manager/order_manager_unit"

echo -e "\nRunning order_manager unit test..."

cd $ROOT
"$TEST_BIN"