#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/builder/builder_dispatch_unit"

echo -e "\nRunning builders unit test..."

cd $ROOT
"$TEST_BIN"