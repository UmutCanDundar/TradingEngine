#!/usr/bin/env bash
set -e

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "RUNNING ALL UNIT TESTS...\n"

"$SOURCE/unit/run_builders_unit.sh"
"$SOURCE/unit/run_network_unit.sh"
"$SOURCE/unit/run_parsers_unit.sh"
"$SOURCE/unit/run_order_manager_unit.sh"
"$SOURCE/unit/run_risk_unit.sh"

