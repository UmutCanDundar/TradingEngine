#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THIRD_PARTY_DIR="$SCRIPT_DIR/third-party"

mkdir -p "$THIRD_PARTY_DIR"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info()    { echo -e "${GREEN}[INFO]${NC} $1"; }
warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

clone_or_skip()
{
    local name=$1
    local url=$2
    local dest=$3
    local extra_args=${4:-""}

    if [ -d "$dest" ]; then
        warning "$name already exists at $dest — skipping."
    else
        info "Cloning $name..."
        git clone --depth=1 $extra_args "$url" "$dest" || error "Failed to clone $name"
        info "$name cloned successfully."
    fi
}

# ── boost ────────────────────────────────────────────────────────────────────
if pkg-config --exists boost 2>/dev/null || [ -d "/usr/include/boost" ]; then
    warning "Boost found on system — skipping clone. CMake will use system boost."
else
    clone_or_skip "boost" \
        "https://github.com/boostorg/boost" \
        "$THIRD_PARTY_DIR/boost" \
        "--recurse-submodules"
fi

# ── abseil-cpp ────────────────────────────────────────────────────────────────
if pkg-config --exists absl_base 2>/dev/null; then
    warning "abseil-cpp found on system — skipping clone. CMake will use system absl."
else
    clone_or_skip "abseil-cpp" \
        "https://github.com/abseil/abseil-cpp" \
        "$THIRD_PARTY_DIR/absl/abseil-cpp"
fi

# ── spdlog ───────────────────────────────────────────────────────────────────
if pkg-config --exists spdlog 2>/dev/null || [ -d "/usr/include/spdlog" ]; then
    warning "spdlog found on system — skipping clone. CMake will use system spdlog."
else
    clone_or_skip "spdlog" \
        "https://github.com/gabime/spdlog" \
        "$THIRD_PARTY_DIR/spdlog"
fi

# ── nlohmann/json ─────────────────────────────────────────────────────────────
if pkg-config --exists nlohmann_json 2>/dev/null || [ -d "/usr/include/nlohmann" ]; then
    warning "nlohmann/json found on system — skipping clone. CMake will use system json."
else
    clone_or_skip "nlohmann/json" \
        "https://github.com/nlohmann/json" \
        "$THIRD_PARTY_DIR/json"
fi

# ── clickhouse-cpp ────────────────────────────────────────────────────────────
if pkg-config --exists clickhouse-cpp 2>/dev/null; then
    warning "clickhouse-cpp found on system — skipping clone. CMake will use system clickhouse-cpp."
else
    clone_or_skip "clickhouse-cpp" \
        "https://github.com/ClickHouse/clickhouse-cpp" \
        "$THIRD_PARTY_DIR/clickhouse/clickhouse-cpp"
fi

# ── folly ─────────────────────────────────────────────────────────────────────
clone_or_skip "folly" \
    "https://github.com/facebook/folly" \
    "$THIRD_PARTY_DIR/folly"

echo ""
info "All dependencies ready. You can now run: ./build.sh"
echo ""
