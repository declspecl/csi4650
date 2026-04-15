#!/usr/bin/env bash

set -e

if [ $# -eq 0 ]; then
    echo "Usage: $0 [build|run|test] [--release]"
    exit 0
fi

TARGET="$1"
FLAG="${2:-}"

BUILD_TYPE="Debug"
BUILD_DIR="build"
if [ "$FLAG" = "--release" ]; then
    BUILD_TYPE="Release"
    BUILD_DIR="build-release"
fi

configure() {
    mkdir -p "$BUILD_DIR"
    cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
}

case "$TARGET" in
    build)
        configure
        echo "Building blackjack_simulator ($BUILD_TYPE)..."
        cmake --build "$BUILD_DIR" --target blackjack_simulator
        ;;
    run)
        configure
        echo "Building blackjack_simulator ($BUILD_TYPE)..."
        cmake --build "$BUILD_DIR" --target blackjack_simulator
        echo "Running blackjack_simulator..."
        "$BUILD_DIR/blackjack_simulator"
        ;;
    test|tests)
        if [ "$FLAG" = "--release" ]; then
            echo "Error: --release is not supported for test (coverage requires Debug)" >&2
            exit 1
        fi
        configure
        echo "Running tests with coverage..."
        cmake --build "$BUILD_DIR" --target coverage
        ;;
    *)
        echo "Usage: $0 [build|run|test] [--release]"
        exit 1
        ;;
esac
