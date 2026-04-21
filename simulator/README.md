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

## Benchmarks

Run with `bash bench/bench.sh`. Results land in `bench/results/`. Generate charts with `python3 bench/plot.py`.

The simulator runs games in parallel using OpenMP. Each game is independent — no shared state between games — so parallelism is embarrassingly parallel across the game dimension.

**Fixed across all benchmarks:** 6-deck shoe, Vegas Strip rules (dealer stands soft 17, double after split allowed), min bet $1, max bet $100, starting bankroll $1000, 100 rounds per game.

---

### Section 1 — Strategy EV Comparison

**What varies:** player strategy (8 strategies tested).
**Fixed:** 50,000 games, 100 rounds per game, 1 thread.

Measures expected value per hand in cents for each strategy. Single-threaded so runtime differences show compute cost of the strategy, not parallelism. Basic strategy and Hi-Lo are the only two that approach or beat the house edge; the rest are negative-EV baselines for comparison.

---

### Section 2 — Strong Scaling (Amdahl's Law)

**What varies:** thread count (1, 2, 3, 4, 6, 8, 10, 12).
**Fixed:** 50,000 games total, 100 rounds per game, basic strategy.

Total work is constant. More threads split the same workload. Speedup should approach linear but serial overhead (deck initialization, RNG seeding, result aggregation) caps it. Measured serial fraction f ≈ 0.074, which puts the theoretical max at ~13.5× — actual measured speedup at 12 threads is ~6.6×.

---

### Section 3 — Weak Scaling (Gustafson's Law)

**What varies:** thread count (1–12) and total games (scaled together: 5,000 games × threads).
**Fixed:** 5,000 games per thread, 100 rounds per game, basic strategy.

Work grows with thread count so each thread always does the same amount. Ideal result: constant wall time regardless of thread count. Actual wall time rises from ~690ms (1 thread) to ~1,400ms (12 threads) because serial overhead (synchronization, aggregation) grows with thread count even though per-thread work stays fixed.

---

### Section 4 — Speedup per Strategy

**What varies:** player strategy (8 strategies), measured at both 1 thread and max threads.
**Fixed:** 50,000 games, 100 rounds per game, 12 threads for the parallel run.

Checks whether all strategies parallelize equally. Strategies with heavier per-hand decision logic (basic, hi-lo) do more work per game — more compute-bound, slightly higher speedup ceiling. Simpler strategies (always-stand, surrender-first) spend less time in decision logic, so their serial overhead is a larger fraction of total time and speedup is slightly lower.

---

### Section 5 — Hardware Counters (perf stat)

**What varies:** thread count (1 thread vs 12 threads).
**Fixed:** 50,000 games, 100 rounds per game, basic strategy.

Raw hardware metrics from `perf stat`. Key numbers:
- **IPC** (instructions per cycle): 2.19 at 1 thread, ~2.0 at 12 threads — slight drop from cache contention.
- **Cache miss rate:** ~71% of cache references miss at both thread counts — workload is memory-bound; games don't share data so L3 pressure stays low but each game's shoe/hand state doesn't fit in L1.
- **CPU utilization:** 1.0 CPUs at 1 thread, 2.2 CPUs at 12 threads — confirms threads are actually running in parallel (would be ~12 with perfect scaling, 2.2 reflects the hybrid P+E core architecture of this CPU).
