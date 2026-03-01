#!/usr/bin/env bash

set -e

mkdir -p build
cmake -S . -B build

TARGET="${1:-all}"

case "$TARGET" in
    test|tests)
        echo "Building test_runner..."
        cmake --build build --target test_runner
        ;;
    main|simulator)
        echo "Building blackjack_simulator..."
        cmake --build build --target blackjack_simulator
        ;;
    all)
        echo "Building all targets..."
        cmake --build build
        ;;
    *)
        echo "Usage: $0 [test|main|all]"
        echo "  test  - Build test_runner"
        echo "  main  - Build blackjack_simulator"
        echo "  all   - Build everything (default)"
        exit 1
        ;;
esac
