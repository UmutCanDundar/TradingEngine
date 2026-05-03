#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/unit/parser/parser_dispatch_unit"

echo -e "\nRunning parsers unit test..."

cd $ROOT
"$TEST_BIN"