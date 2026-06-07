#!/bin/bash

error() {
    echo "Error: $1" >&2
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

CXX_COMPILER=$(compgen -c | grep -E '^g\+\+-([0-9]+)$' | sort -t- -k2 -rn | head -1)
CXX_COMPILER=$(command -v "$CXX_COMPILER" 2>/dev/null)
VER=$(echo "$CXX_COMPILER" | grep -oE '[0-9]+$')

if [ -z "$CXX_COMPILER" ] || [ "${VER:-0}" -lt 13 ]; then
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
    local extra_args=("$@")
    local cache="$BUILD_DIR/CMakeCache.txt"
    local needs_configure=0

    if [ ! -f "$cache" ]; then
        needs_configure=1
    else
        local cached_type
        cached_type=$(grep -oP '(?<=CMAKE_BUILD_TYPE:STRING=).*' "$cache" 2>/dev/null || true)
        if [ "$cached_type" != "$build_type" ]; then
            echo "Build type changed: '$cached_type' -> '$build_type'"
            needs_configure=1
        fi

        local cached_cxx
        cached_cxx=$(grep -oP '(?<=CMAKE_CXX_COMPILER:FILEPATH=).*' "$cache" 2>/dev/null || true)
        if [ "$cached_cxx" != "$CXX_COMPILER" ]; then
            needs_configure=1
        fi

        local cached_asan
        cached_asan=$(grep -oP '(?<=ENABLE_ASAN:BOOL=).*' "$cache" 2>/dev/null || true)
        local want_asan="OFF"
        for arg in "${extra_args[@]}"; do
            [[ "$arg" == "-DENABLE_ASAN=ON" ]] && want_asan="ON"
        done
        if [ "${cached_asan^^}" != "${want_asan^^}" ]; then
            needs_configure=1
        fi

        local cached_tsan
        cached_tsan=$(grep -oP '(?<=ENABLE_TSAN:BOOL=).*' "$cache" 2>/dev/null || true)
        local want_tsan="OFF"
        for arg in "${extra_args[@]}"; do
            [[ "$arg" == "-DENABLE_TSAN=ON" ]] && want_tsan="ON"
        done
        if [ "${cached_tsan^^}" != "${want_tsan^^}" ]; then
            needs_configure=1
        fi
    fi

    if [ "$needs_configure" -eq 1 ]; then
        if [ -f "$cache" ]; then
            rm -f "$cache"
            rm -f "$BUILD_DIR/CMakeFiles/cmake.check_cache"
        fi

        cmake -B "$BUILD_DIR" -G Ninja \
            -DCMAKE_BUILD_TYPE="$build_type" \
            -DCMAKE_CXX_COMPILER="$CXX_COMPILER" \
            -Wno-dev \
            "${extra_args[@]}" \
            .
    fi
}

case "$MODE" in
    release)        configure "Release"         ;;
    debug)          configure "Debug"           ;;
    relwithdebinfo) configure "RelWithDebInfo"  ;;
    asan)           configure "Debug"          -DENABLE_ASAN=ON  -DENABLE_TSAN=OFF ;;
    tsan)           configure "RelWithDebInfo" -DENABLE_ASAN=OFF -DENABLE_TSAN=ON  ;;
    *)              error "Unknown mode: $MODE"; usage; exit 1 ;;
esac

cmake --build "$BUILD_DIR" -j"$CORES" -- -k 0
BUILD_EXIT=$?

if [ $BUILD_EXIT -ne 0 ]; then
    echo ""
    echo "Build finished with errors (exit code: $BUILD_EXIT)."
    exit $BUILD_EXIT
else
    echo "Build successful."
fi