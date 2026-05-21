#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

CXX_COMPILER=$(compgen -c | grep -E '^g\+\+-([0-9]+)$' | sort -t- -k2 -rn | head -1)
CXX_COMPILER=$(command -v "$CXX_COMPILER" 2>/dev/null)
VER=$(echo "$CXX_COMPILER" | grep -oE '[0-9]+$')
if [ -z "$CXX_COMPILER" ] || [ "$VER" -lt 13 ]; then
    echo "GCC 13+ required." && exit 1
fi

CORES=$(( $(nproc) > 4 ? $(nproc) - 2 : $(nproc) ))

usage()
{
    echo ""
    echo "Usage: ./build.sh [mode]"
    echo ""
    echo "  Modes:"
    echo "    release        Release build (default)"
    echo "    debug          Debug build"
    echo "    relwithdebinfo RelWithDebInfo build"
    echo "    asan           Debug + AddressSanitizer"
    echo "    tsan           RelWithDebInfo + ThreadSanitizer"
    echo ""
}

MODE=${1:-release}

configure()
{
    local build_type=$1
    shift

    cmake -B "$BUILD_DIR" -G Ninja \
          -DCMAKE_BUILD_TYPE="$build_type" \
          -DCMAKE_CXX_COMPILER="$CXX_COMPILER" \
          -Wno-dev \
          "$@" \
          .
}

case "$MODE" in
    release)        configure "Release" "-DENABLE_ASAN=OFF -DENABLE_TSAN=OFF" ;;
    debug)          configure "Debug" "-DENABLE_ASAN=OFF -DENABLE_TSAN=OFF" ;;
    relwithdebinfo) configure "RelWithDebInfo" "-DENABLE_ASAN=OFF -DENABLE_TSAN=OFF";;
    asan)           configure "Debug" "-DENABLE_ASAN=ON -DENABLE_TSAN=OFF" ;;
    tsan)           configure "RelWithDebInfo" "-DENABLE_ASAN=OFF -DENABLE_TSAN=ON" ;;
    *)              error "Unknown mode: $MODE"; usage ;;
esac

cmake --build "$BUILD_DIR" -j"$CORES" 
