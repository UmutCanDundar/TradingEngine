#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
TEST_BIN="$ROOT/build/test/perf/parser/parser_dispatch_perf"
RESULT_DIR="$ROOT/test/_results/perf"
RESULT="$RESULT_DIR/parser_results.txt"
PERF_DATA_DIR="$RESULT_DIR/perf_data"

mkdir -p "$RESULT_DIR"
mkdir -p "$PERF_DATA_DIR"

RUN_TIME="$(date '+%Y-%m-%d %H:%M:%S')"

BLUE='\033[0;34m'
NC='\033[0m'
echo -e "\n\n${BLUE}Running parser perf test...${NC}"

run_section() {
    echo ""
    echo "=================================================="
    echo "$1"
    echo "=================================================="
}

exec > >(tee -a "$RESULT") 2>&1

echo "##################################################"
echo "RUN START: $RUN_TIME"
echo "##################################################"
echo ""
echo "================ SYSTEM INFO ================"
echo "KERNEL: $(uname -r)"
echo "ARCH: $(uname -m)"
LANG=C lscpu | grep -E "Model name|CPU\(s\)|Thread|Core|Socket|MHz" || true
echo ""
echo "Governor:"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "N/A"
echo ""
echo "NUMA INFO:"
numactl --hardware 2>/dev/null || echo "N/A"

run_section "1. PERF STAT - BASE"
perf stat -d "$TEST_BIN" 2>&1 || true

run_section "2. PERF STAT - CPU + BRANCH"
perf stat \
  -e cpu_core/cycles/,cpu_core/instructions/,cpu_core/branches/,cpu_core/branch-misses/ \
  -e cpu_atom/cycles/,cpu_atom/instructions/,cpu_atom/branches/,cpu_atom/branch-misses/ \
  "$TEST_BIN" 2>&1 || true

run_section "3. PERF STAT - DCACHE"
perf stat -e cpu_core/L1-dcache-load-misses/,cpu_core/LLC-load-misses/ "$TEST_BIN" 2>&1 || true

run_section "4. PERF STAT - ICACHE"
perf stat -e cpu_core/L1-icache-load-misses/ "$TEST_BIN" 2>&1 || true

run_section "5. PERF STAT - TLB"
perf stat -e cpu_core/dTLB-load-misses/,cpu_core/iTLB-load-misses/ "$TEST_BIN" 2>&1 || true

run_section "6. PERF RECORD - CPU + BRANCH (>1% || TOP 20)"
perf record -F 99999 -g -m 256M \
  -e cpu_core/cycles/,cpu_core/instructions/,cpu_core/branches/,cpu_core/branch-misses/ \
  -e cpu_atom/cycles/,cpu_atom/instructions/,cpu_atom/branches/,cpu_atom/branch-misses/ \
    -o "$PERF_DATA_DIR/parser_cpu.data" "$TEST_BIN" >/dev/null 2>&1 || true
perf report --stdio -i "$PERF_DATA_DIR/parser_cpu.data" \
    --call-graph graph,0.5,caller,function,percent 2>/dev/null \
    | grep -E "^\s+[0-9]+\.[0-9]+%" \
    | awk '{match($0,/[0-9]+\.[0-9]+/); if (substr($0,RSTART,RLENGTH)+0 >= 1.0) print}' \
    | head -20 || true

run_section "7. PERF RECORD - DCACHE (>1% || TOP 20)"
perf record -F 99999 -g -m 256M -e cpu_core/L1-dcache-load-misses/,cpu_core/LLC-load-misses/ \
    -o "$PERF_DATA_DIR/parser_dcache.data" "$TEST_BIN" >/dev/null 2>&1 || true
perf report --stdio -i "$PERF_DATA_DIR/parser_dcache.data" 2>/dev/null \
    | grep -E "^\s+[0-9]+\.[0-9]+%" \
    | awk '{match($0,/[0-9]+\.[0-9]+/); if (substr($0,RSTART,RLENGTH)+0 >= 1.0) print}' \
    | head -20 || true

run_section "8. PERF RECORD - ICACHE (>1% || TOP 20)"
perf record -F 99999 -g -m 256M -e cpu_core/L1-icache-load-misses/ \
    -o "$PERF_DATA_DIR/parser_icache.data" "$TEST_BIN" >/dev/null 2>&1 || true
perf report --stdio -i "$PERF_DATA_DIR/parser_icache.data" 2>/dev/null \
    | grep -E "^\s+[0-9]+\.[0-9]+%" \
    | awk '{match($0,/[0-9]+\.[0-9]+/); if (substr($0,RSTART,RLENGTH)+0 >= 1.0) print}' \
    | head -20 || true

run_section "9. PERF RECORD - TLB (>1% || TOP 20)"
perf record -F 99999 -g -m 256M -e cpu_core/dTLB-load-misses/,cpu_core/iTLB-load-misses/ \
    -o "$PERF_DATA_DIR/parser_tlb.data" "$TEST_BIN" >/dev/null 2>&1 || true
perf report --stdio -i "$PERF_DATA_DIR/parser_tlb.data" 2>/dev/null \
    | grep -E "^\s+[0-9]+\.[0-9]+%" \
    | awk '{match($0,/[0-9]+\.[0-9]+/); if (substr($0,RSTART,RLENGTH)+0 >= 1.0) print}' \
    | head -20 || true

run_section "10. C2C - FALSE SHARING SUMMARY"
perf c2c record -g --call-graph dwarf \
    -o "$PERF_DATA_DIR/parser_c2c.data" -- "$TEST_BIN" >/dev/null 2>&1 || true
perf c2c report --stdio --stats \
    -i "$PERF_DATA_DIR/parser_c2c.data" 2>/dev/null || true

run_section "11. FLAMEGRAPH"
if command -v stackcollapse-perf.pl &>/dev/null && command -v flamegraph.pl &>/dev/null; then
    perf record -F 99999 -g -m 256M  \
        -e cpu_core/cycles/ \
        -o "$PERF_DATA_DIR/parser_flame.data" -- "$TEST_BIN" >/dev/null 2>&1 || true
    perf script -i "$PERF_DATA_DIR/parser_flame.data" \
        | stackcollapse-perf.pl | flamegraph.pl \
        > "$RESULT_DIR/flamegraph_parser.svg"
    echo "Flamegraph saved: $RESULT_DIR/flamegraph_parser.svg"
else
    echo "FlameGraph tools not found — skipping."
fi

echo ""
echo "================ END RUN ================"
echo ""
echo "Perf data files: $PERF_DATA_DIR"
echo "Results: $RESULT"