#!/usr/bin/env bash
set -e
SOURCE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "\n${YELLOW}RUNNING ALL UNIT TESTS...${NC}"

"$SOURCE/unit/run_builders_unit.sh"
"$SOURCE/unit/run_network_unit.sh"
"$SOURCE/unit/run_parsers_unit.sh"
"$SOURCE/unit/run_order_manager_unit.sh"
"$SOURCE/unit/run_risk_unit.sh"
