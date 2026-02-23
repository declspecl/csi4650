# Blackjack Simulator

## How to Build

Requires:
- CMake 3.20+
- clang++
    - If on macOS, install LLVM with homebrew with `brew install llvm`.
- OpenMP

```bash
./compile.sh
```

Run with `build/blackjack_simulator`

## OpenMP

Install OpenMP with your system's package manager, CMake is already configured to find it.
CMake also spits out `build/compile_commands.json` which can be used for LSP support, otherwise your editor won't be able to resolve `omp.h`.
For VSCode, can install the `clangd` extension and it will automatically pick up the config and resolve OpenMP.
