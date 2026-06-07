#!/usr/bin/env bash
set -e

SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "\n${YELLOW}RUNNING ALL UNIT TESTS...${NC}"

"$SOURCE/unit/run_builder_unit.sh" || true
"$SOURCE/unit/run_network_unit.sh" || true
"$SOURCE/unit/run_parser_unit.sh" || true
"$SOURCE/unit/run_order_manager_unit.sh" || true
"$SOURCE/unit/run_risk_unit.sh" || true
