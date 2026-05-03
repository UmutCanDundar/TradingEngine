#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/network/network_unit"

echo -e "\nRunning network unit test..."

cd $ROOT
"$TEST_BIN"