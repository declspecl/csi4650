#set document(
  title: "Parallel Blackjack Simulator: Final Report",
  author: ("Gavin D'Hondt", "Anton Sakhanovych"),
)

#set page(
  paper: "us-letter",
  margin: (x: 1.25in, y: 1in),
  numbering: "1",
  number-align: center,
)

#set text(
  font: "New Computer Modern",
  size: 11pt,
  lang: "en",
)

#set par(
  justify: true,
  leading: 0.65em,
  first-line-indent: 1.2em,
)

#set heading(numbering: "1.1")

#show heading.where(level: 1): it => {
  v(1.2em)
  text(size: 13pt, weight: "bold", it)
  v(0.4em)
}

#show heading.where(level: 2): it => {
  v(0.8em)
  text(size: 11.5pt, weight: "bold", it)
  v(0.3em)
}

#show raw: it => box(
  fill: luma(245),
  inset: (x: 4pt, y: 2pt),
  radius: 2pt,
  text(size: 9.5pt, it)
)


#align(center)[
  #v(0.5in)
  #text(size: 18pt, weight: "bold")[Parallel Blackjack Simulator]
  #v(0.3em)
  #text(size: 13pt)[Final Report]
  #v(1em)
  #text(size: 11pt)[
    Gavin D'Hondt #h(1.5em) Anton Sakhanovych \
    CSI 4650, Parallel Computing, Dr. Tian \
    Oakland University #sym.dot.c April 2026
  ]
  #v(0.5em)
  #line(length: 60%, stroke: 0.5pt)
]


#v(1em)
#align(center)[*Abstract*]

#par(first-line-indent: 0pt)[
  We built a parallel Monte Carlo blackjack simulator in C++23 with OpenMP and used it to answer two questions: what is the expected value of eight different playing strategies under Vegas Strip rules, and how well does the workload scale across threads? Strong scaling on a 12-thread Intel i7-1365U peaked at 6.62 #sym.times speedup; the same binary on an Apple M4 hit 6.89 #sym.times with a lower measured serial fraction, which turns out to be a hardware topology effect rather than a code difference. Only Basic (+0.09 #sym.cent/hand) and Hi-Lo (+0.45 #sym.cent/hand) beat the house. Weak scaling is poor: Gustafson efficiency falls to 4.1% at 12 threads, and the flat OpenMP reduction at thread join is the main reason why.
]

#v(1em)


= Introduction

Blackjack occupies a useful position as a parallel benchmark: the per-game simulation is computationally non-trivial (card shuffle, RNG, multi-hand decision trees, bankroll accounting), yet individual games are fully independent given private shoe and bankroll state. This makes it a natural fit for data-parallel execution with a scalar reduction at join, a map-reduce shape that exposes the serial fraction through Amdahl's Law clearly and without confounding inter-thread communication.

The project has two parallel goals. First, we want to quantify the expected value of eight strategies ranging from provably optimal (Hi-Lo card counting, Basic strategy) to deliberately pathological baselines (Double First, Bullish), using Vegas Strip rules as a fixed evaluation environment. Second, we want to characterize how the outer-loop parallelization scales on real hardware, including a hybrid CPU with heterogeneous core performance and a high-performance homogeneous design.

The simulator runs 50,000 games of 100 rounds each as its standard benchmark workload. Measurements were taken on a laptop-class Intel i7-1365U (voidheart, Void Linux) and an Apple M4 MacBook using the same binary compiled with `-O3 -march=native`. All results are reproducible: the seeding scheme is deterministic and independent of thread count, so the same statistical outcomes emerge whether 1 or 12 threads are used.


= Related Work

== Blackjack Strategy

The mathematical basis for card counting goes back to Edward Thorp's work in the early 1960s, which showed that a point-count system tracking high and low cards could give the player a small positive edge. The canonical multi-deck basic strategy tables were produced by Julian Braun at IBM using exhaustive Monte Carlo simulation and are still the standard reference today; our `BasicStrategy` implementation follows those tables directly.

== Parallel Monte Carlo and OpenMP

Parallel Monte Carlo is straightforward in structure: each thread simulates an independent partition of trials and contributes to a shared reduction at the end. The main engineering challenges are avoiding shared RNG state and keeping synchronization cost low. The usual approach is per-thread RNG seeding, but this project instead derives the seed from the game and round index directly, so any thread can produce the same shuffle for any game without coordination. The tradeoff with OpenMP's flat `reduction` clause is that the merge at thread join runs in O(N) time, which hurts weak scaling, a known limitation of the flat reduction pattern that we discuss further in @sec-weak-scaling.


= Design and Implementation

== Architecture Overview

The simulator is implemented as a header-only C++23 library under `include/blackjack/`, with a single translation unit (`src/main.cpp`) providing the entry point, CLI parser, and top-level simulation driver. The build system is CMake 3.20+ targeting clang++ with C++23 and OpenMP; release builds add `-O3 -march=native` for platform-specific vectorization. A GoogleTest suite covers correctness at the unit level, built with LLVM coverage instrumentation in debug mode.

The library is organized into five namespaces mirroring the logical domain:

#v(0.3em)
#table(
  columns: (1.4fr, 2.6fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (left, left),
  [*Namespace*], [*Responsibility*],
  [`blackjack::card`],   [Card rank, suit, and value encoding],
  [`blackjack::deck`],   [`Deck` (52-card) and `Shoe` (up to 6 decks) with Fisher-Yates shuffle],
  [`blackjack::hand`],   [Hand accumulation, soft/hard value, origin (split), outcome tags],
  [`blackjack::game`],   [`Game`, `Ruleset`, `BettingConfig`, `GameStatistics`: full round lifecycle],
  [`blackjack::player`], [`Player` bankroll, `PlayerStrategy` hierarchy, `GameContext` snapshot],
)
#v(0.3em)

All hot-path types are stack-allocated. `Game` holds a `std::array<Player, 7>` and a `Shoe` inline, keeping the entire game state in a contiguous block that fits in a few cache lines.

== Ruleset and Betting Configuration

The `Ruleset` struct encodes six rule parameters: blackjack payout ratio (3:2 default), dealer hits soft 17, late surrender, double-after-split, and maximum split hands. Two named presets are provided: `Ruleset::default_vegas()` (3:2, S17, surrender allowed, DAS, 4 splits) used throughout the benchmark, and `tight_h17_no_surrender()` (6:5, H17, no surrender, no DAS, 2 splits) as a second evaluation environment.

`BettingConfig` encodes table limits: minimum bet (\$1), maximum bet (\$100), and initial bankroll (\$1000), all stored in cents as `uint32_t` to avoid floating-point representation errors during bankroll arithmetic. Integer cent arithmetic is used everywhere; EV is computed as a `double` only at aggregation time.

== Strategy Hierarchy

All eight player strategies derive from the abstract `PlayerStrategy` base class, which exposes two virtual methods:

```cpp
Decision get_decision(const GameContext& ctx) const noexcept;
uint32_t get_bet_size(float true_count,
                      const BettingConfig& cfg) const noexcept;
```

`get_bet_size` has a default implementation returning `cfg.get_min_bet()`, so flat-bet strategies only need to override `get_decision`. `GameContext` is an immutable value snapshot passed per decision call; it carries the player's hand, the dealer's upcard, the `LegalActions` bitmask (`can_double`, `can_split`, `can_surrender`), the running Hi-Lo count, and the number of cards remaining in the shoe. This design keeps strategy logic stateless and trivially testable.

`BasicStrategy` implements the full multi-deck S17/DAS table with three private helpers: `should_split`, `soft_decision`, and `hard_decision`. All branches resolve to compile-time `switch` or comparison chains; no heap allocation or table lookup is needed. `HiLoStrategy` wraps `BasicStrategy` and overrides nine index plays (true-count deviations on hard 16 vs 10, hard 15 vs 10, hard 12 vs 2/3, hard 10 vs 10/A, and hard 9 vs 2/7) before delegating to `BasicStrategy`. Bet sizing scales linearly with true count, capped at 8 #sym.times the minimum bet.

The six baseline strategies range in complexity from `AlwaysStandStrategy` (single unconditional return) to `DoubleFirstStrategy` (doubles whenever `can_double`, otherwise follows a basic-like fallback), providing a spread of per-hand compute costs for the scaling experiments.

== Round Lifecycle

Each call to `Game::play_round(seed)` executes a full blackjack round in seven steps:

+ *Reshuffle check:* If fewer than 52 cards remain in the shoe or the shoe has never been shuffled, `shoe.shuffle(seed)` is called and the Hi-Lo running count resets to zero.
+ *Bet collection:* Each active player queries `strategy->get_bet_size(true_count, betting)` and the bet is deducted from their bankroll.
+ *Initial deal:* Two cards are dealt to each player and the dealer, interleaved in the standard two-pass order. Each draw updates the running count via Hi-Lo tagging (low cards 2-6 increment; high cards 10-A decrement).
+ *Player turns:* For each player and each active hand, the game loops calling `strategy->get_decision(ctx)` and applying the result until the hand terminates (bust, stand, double, surrender, or blackjack).
+ *Dealer turn:* The dealer strategy (`DealerStrategy`) hits until reaching hard 17 or better.
+ *Resolution:* Each player hand is compared against the dealer hand; payouts follow the ruleset (3:2 for blackjack, 1:1 for regular win, 0 for loss, half-bet return for surrender).
+ *Statistics update:* Hands played and bankroll totals accumulate in the `Game` object.

== Parallelization Design

The parallel structure is a single `#pragma omp parallel for` over the game index with a scalar reduction at join:

```cpp
#pragma omp parallel for num_threads(cfg.threads)      \
    reduction(+: total_hands, total_starting, total_ending)
for (uint32_t g = 0; g < cfg.game_count; g++) {
    for (uint32_t r = 0; r < cfg.rounds_per_game; r++)
        games[g].play_round(round_seed(cfg, g, r));
    GameStatistics stats = games[g].aggregate_statistics();
    total_hands    += stats.get_hands_played();
    total_starting += stats.get_starting_bankroll();
    total_ending   += stats.get_ending_bankroll();
}
```

Parallelism lives entirely at the game granularity. The inner round loop remains serial per game, keeping shoe state, bankroll history, and hand context coherent within a single thread at all times. This eliminates all intra-game data hazards without any locking. The only synchronization point is the OpenMP reduction at thread join, which executes once per thread rather than once per hand.

== Seeding and Reproducibility

Round seeds are computed as:

#v(0.3em)
#align(center)[`round_seed = base_seed + game_index * rounds_per_game + round_index`]
#v(0.3em)

This seed is passed into every `play_round` call, but the shoe is only actually reshuffled when cards remaining drops below 52 (or on the very first round). When a reshuffle does happen, it uses the seed for that specific round, which is a deterministic function of position in the work space rather than which thread is running it. Results are therefore bit-identical across all thread counts for a given `(base_seed, game_count, rounds_per_game, strategy)` tuple, which is essential for validating that the parallel and serial paths produce the same outcomes.


= Experimental Setup

== Hardware

#v(0.3em)
#table(
  columns: (1.2fr, 1.4fr, 1.4fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (left, left, left),
  [*Parameter*], [*Intel i7-1365U (primary)*], [*Apple M4 (cross-platform)*],
  [Core topology], [2 P-cores + 8 E-cores, 12 logical threads], [10 P-cores, 10 logical threads],
  [P-core frequency], [#sym.lt.eq 3.9 GHz (boost)], [#sym.lt.eq 4.4 GHz (boost)],
  [E-core frequency], [#sym.lt.eq 2.7 GHz], [N/A (homogeneous)],
  [Last-level cache], [12 MB shared L3], [16 MB shared L3],
  [DRAM], [16 GB LPDDR5], [16 GB unified LPDDR5X],
  [OS], [Void Linux (6.12 kernel)], [macOS Sequoia],
)
#v(0.3em)

The i7-1365U's hybrid topology is the primary subject of analysis: its 2 P-cores and 8 E-cores share the same logical thread count as a homogeneous 12-core design but deliver substantially different per-thread throughput.

== Build and Runtime Configuration

All benchmarks use the release build: clang++, C++23, `-O3 -march=native -g` (debug symbols for `perf`). The test binary is built separately with `-fprofile-instr-generate -fcoverage-mapping` for coverage analysis and is never used for timing. OpenMP is linked via `find_package(OpenMP)` using the system LLVM runtime.

== Benchmark Parameters

All runs use Vegas Strip rules (`Ruleset::default_vegas()`): 3:2 blackjack payout, dealer stands on soft 17, late surrender allowed, double-after-split allowed, up to 4 split hands. Betting parameters are fixed at min bet \$1, max bet \$100, initial bankroll \$1000. The base seed is 0 for all benchmark runs.

#v(0.3em)
#table(
  columns: (1.5fr, 1fr, 2.5fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (left, left, left),
  [*Experiment*], [*Games #sym.times Rounds*], [*Variable*],
  [EV comparison],          [50,000 #sym.times 100], [Strategy (8 strategies), 1 thread],
  [Strong scaling],         [50,000 #sym.times 100], [Thread count: 1 #sym.dash.en 12, basic strategy],
  [Weak scaling],           [5,000/thread #sym.times 100], [Thread count: 1 #sym.dash.en 12 (total work scales), basic strategy],
  [Per-strategy speedup],   [50,000 #sym.times 100], [Strategy (8 strategies), 1T vs 12T],
  [Hardware counters],      [50,000 #sym.times 100], [`perf stat`, basic strategy, 1T and 12T],
)
#v(0.3em)

Hardware counter measurements used `perf stat -e cycles,instructions,cache-misses,cache-references,task-clock` on the i7-1365U. The `perf` tool reports aggregate counts across all threads; CPUs utilized is derived from `task-clock / wall_clock`.


= Strategy Results and Expected Value

== Eight Strategies

The simulator evaluates eight strategies, grouped by design philosophy:

#v(0.3em)
#table(
  columns: (1.6fr, 1fr, 2.4fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (left, right, left),
  [*Strategy*], [*EV/hand (#sym.cent)*], [*Notes*],
  [hi-lo],          [+0.45],  [Card counting; count-sensitive deviations + bet sizing],
  [basic],          [+0.09],  [Optimal static play; dealer upcard, soft totals, doubles, splits, surrender],
  [mimic-dealer],   [#sym.minus 5.71],  [Copies dealer rules; ignores profitable deviations],
  [bearish],        [#sym.minus 7.78],  [Conservative heuristic; sacrifices double/split value],
  [always-stand],   [#sym.minus 15.79], [Never hits; leaves dealer busts unexploited],
  [surrender-first],[#sym.minus 40.65], [Surrenders whenever legal; gives up too much EV],
  [bullish],        [#sym.minus 64.13], [Hits aggressively without counting; overbets],
  [double-first],   [#sym.minus 67.30], [Doubles on every eligible hand; worst EV],
)
#v(0.3em)

_Benchmark conditions: 50,000 games #sym.times 100 rounds, 1 thread, Vegas Strip rules (six-deck shoe, dealer stands on soft 17, late surrender allowed)._

== Interpretation

Basic strategy achieves near-zero house edge because it encodes the complete decision tree conditioned on dealer upcard, hard and soft hand totals, splitting rules (including DAS), and surrender. Every deviation from this tree, however intuitive, increases the house edge. Hi-Lo extends Basic by introducing true-count deviations on nine borderline hands and scaling bet size proportionally to the true count up to an 8 #sym.times cap; the positive edge (+0.45 #sym.cent) emerges from the combination of reduced house edge on borderline hands when the remaining shoe is ten-rich and increased bet sizing during those favorable shoe compositions.

The middle tier (mimic-dealer, bearish) reveals the cost of ignoring profitable actions: mimic-dealer loses 5.71 #sym.cent/hand despite never making an obviously irrational play, because it gives up double and split equity that basic strategy captures. The bottom four strategies are instructive stress tests. Double First and Bullish produce the highest per-hand computation (more doubling decisions, more card draws) while simultaneously losing the most money, a point we return to in the per-strategy speedup analysis.

Mimic-dealer is also worth calling out: it plays exactly like the dealer but has no hole-card advantage. Its negative EV shows that the house edge comes from players busting before the dealer even plays, not from any rule asymmetry.


= Strong Scaling: Amdahl's Law

== Intel i7-1365U Results

#v(0.3em)
#table(
  columns: (1fr, 1.5fr, 1fr, 1fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (center, right, right, right),
  [*Threads*], [*Wall time (ms)*], [*Speedup*], [*Efficiency*],
  [1],  [6833.71], [1.000 #sym.times], [100.0%],
  [2],  [3456.76], [1.977 #sym.times], [98.8%],
  [3],  [3279.54], [2.084 #sym.times], [69.5%],
  [4],  [2648.56], [2.580 #sym.times], [64.5%],
  [6],  [1850.84], [3.692 #sym.times], [61.5%],
  [8],  [1468.98], [4.652 #sym.times], [58.2%],
  [10], [1213.38], [5.632 #sym.times], [56.3%],
  [12], [1032.38], [6.619 #sym.times], [55.2%],
)
#v(0.3em)

The measured serial fraction is $f approx 0.074$, giving an Amdahl theoretical maximum of $1/f approx 13.5times$. The actual peak at 12 threads is 6.62 #sym.times, leaving the program well below its theoretical ceiling. The most striking feature of this table is the efficiency cliff between 2 and 3 threads: efficiency drops from 98.8% to 69.5% in a single step. This is a topology artifact: the i7-1365U exposes exactly 2 P-cores with hyperthreading disabled for OpenMP scheduling purposes, so threads 1 and 2 land on the high-performance P-cores. Thread 3 is the first to be assigned to an E-core running at ~2.3 GHz rather than the P-core's 3.8 GHz.

Beyond 3 threads, efficiency continues declining gradually (69.5% #sym.arrow.r 55.2%) as more work shifts to the 8 lower-throughput E-cores. OpenMP's default work-stealing scheduler distributes game iterations uniformly across all 12 logical threads without awareness of the performance asymmetry.

== Cross-Platform Comparison with Apple M4

The same binary on an Apple M4 (10 homogeneous P-cores) produces significantly different scaling behavior:

- 1-thread baseline: 4,799 ms (1.42 #sym.times faster per core than the i7)
- 10-thread peak: 696 ms, speedup 6.89 #sym.times
- Serial fraction: $f = 0.050$, theoretical max 20 #sym.times
- Efficiency at 10T: 68.9%

The lower serial fraction on M4 (5.0% vs 7.4%) is not a code difference; it is a hardware effect. Amdahl's $f$ is not a property of the program in isolation; it is a joint property of the program and the machine. On the i7, the E-cores contribute real thread count but execute at roughly half the P-core IPC-adjusted throughput. OpenMP treats all 12 threads as equal work units, but they are not, so the effective serial fraction appears inflated when measured against the homogeneous 1-thread baseline. The `perf stat` data confirms this: on 12 threads, the i7 reports only 2.25 CPUs utilized, meaning the remaining 9.75 logical threads deliver work equivalent to roughly 1.25 additional P-core equivalents.

The M4's higher per-core throughput and uniform core performance allow efficiency to decline more gracefully (68.9% at the thread ceiling) compared to the i7's 55.2% at its ceiling.


= Weak Scaling: Gustafson's Law <sec-weak-scaling>

In the weak scaling experiment, per-thread work is held constant at 5,000 games while the total game count scales linearly with thread count. Under perfect weak scaling, wall time should remain constant at the 1-thread baseline of 690 ms regardless of how many threads are added.

#v(0.3em)
#table(
  columns: (1fr, 1.2fr, 1.5fr, 1.8fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (center, right, right, right),
  [*Threads*], [*Total games*], [*Wall time (ms)*], [*Gustafson eff.*],
  [1],  [5,000],  [690],  [100.0%],
  [2],  [10,000], [698],  [49.4%],
  [3],  [15,000], [981],  [23.5%],
  [4],  [20,000], [993],  [17.4%],
  [6],  [30,000], [1135], [10.1%],
  [8],  [40,000], [1183], [7.3%],
  [10], [50,000], [1214], [5.7%],
  [12], [60,000], [1408], [4.1%],
)
#v(0.3em)

Wall time climbs from 690 ms to 1,408 ms as thread count increases from 1 to 12, a 2 #sym.times increase despite 12 #sym.times the total work. Gustafson efficiency collapses to 4.1% at the maximum thread count.

The primary cause is structural: the OpenMP `reduction(+:...)` clause performs a flat merge where each thread's partial sum is accumulated serially into the shared totals at join. Reduction cost therefore grows as O(N) in the number of threads, even when per-thread parallel work is held constant. A tree-structured hierarchical merge would replace O(N) serial accumulation with O(log N) depth; at 12 threads this would reduce the serial merge from 12 sequential additions to 4 levels of 2-way combines. The second contributing factor is the same P-core/E-core performance asymmetry identified in the strong scaling analysis: as total game count grows, more iterations must be processed by slower E-cores, causing the join barrier to be held up by the slowest thread.

The step from 1 to 2 threads already breaks perfect weak scaling (efficiency drops to 49.4%), suggesting the serial overhead at join is not negligible even at small thread counts. This is consistent with the theory: OpenMP fork/join overhead, thread-local `Game` object construction, and the reduction itself are all fixed per-run costs that do not scale with per-thread work.


= Per-Strategy Parallel Speedup

== 12-Thread Speedup by Strategy

#v(0.3em)
#table(
  columns: (1.8fr, 1fr, 1.2fr, 1fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (left, right, right, right),
  [*Strategy*],       [*1T (ms)*], [*12T (ms)*], [*Speedup*],
  [bullish],          [7301],      [1123],        [6.50 #sym.times],
  [mimic-dealer],     [6721],      [1042],        [6.45 #sym.times],
  [basic],            [6845],      [1065],        [6.43 #sym.times],
  [double-first],     [5397],      [868],         [6.22 #sym.times],
  [bearish],          [5041],      [818],         [6.16 #sym.times],
  [hi-lo],            [7634],      [1260],        [6.06 #sym.times],
  [surrender-first],  [3917],      [646],         [6.06 #sym.times],
  [always-stand],     [4432],      [806],         [5.50 #sym.times],
)
#v(0.3em)

The variance in speedup across strategies is modest (5.50 #sym.times to 6.50 #sym.times), but the ordering is meaningful. `always-stand` achieves the lowest speedup (5.50 #sym.times) because its decision logic is trivially short: every hand resolves in a single step with no card draws beyond the initial two, so compute-to-overhead ratio is at its minimum. Thread launch and reduction costs constitute a larger fraction of total runtime for lightweight strategies.

`bullish` and `mimic-dealer` involve more card draws per hand because they hit frequently, producing a higher compute-to-overhead ratio and correspondingly better scaling. Interestingly, `hi-lo`, the best-performing strategy by EV at +0.45 #sym.cent/hand, achieves only 6.06 #sym.times speedup, tied with `surrender-first`. This occurs because Hi-Lo's additional computation (true-count calculation, nine deviation checks, bet-size computation involving a `std::min`) is concentrated in sequential branches that cannot be vectorized across hands, whereas the extra card draws in `bullish` produce more uniform, easily parallelizable work.

It is worth noting that compute cost and EV have basically no correlation. The worst strategy (double-first, #sym.minus 67.30 #sym.cent/hand) scales to 6.22 #sym.times because doubling generates extra card draws that parallelize well, while hi-lo (the best by EV) only hits 6.06 #sym.times. Running the benchmark with only the best strategy would understate scalability by around 15%.


= Hardware Counter Analysis

== `perf stat` Results (basic strategy, 50,000 games)

#v(0.3em)
#table(
  columns: (2.2fr, 1.5fr, 1.5fr),
  stroke: 0.4pt,
  inset: 7pt,
  align: (left, right, right),
  [*Metric*], [*1 thread*], [*12 threads*],
  [P-core cycles],         [51.7 B],   [66.8 B],
  [P-core instructions],   [113 B],    [132 B],
  [P-core IPC],            [2.19],     [1.98],
  [E-core IPC],            [1.08],     [1.28],
  [Cache misses (total)],  [5.14 M],   [10.6 M],
  [Miss rate],             [71.2%],    [68.7%],
  [CPUs utilized],         [1.00],     [2.25],
)
#v(0.3em)

The 71% L1 cache miss rate looks bad, but the IPC numbers tell a different story: IPC holds at 2.19 single-threaded and only drops to 1.98 under 12 threads. The hot path is compute-dense arithmetic over private per-thread state (shoe array, hand value accumulation, RNG state), with cache misses being absorbed by the out-of-order execution window. Misses are thus amortized across a large instruction window rather than causing visible stalls.

The most telling number is CPUs utilized: 2.25 out of 12. This number directly explains the gap between ideal 12 #sym.times scaling and the observed 6.6 #sym.times. The 8 E-cores are contributing approximately 0.25 effective P-core equivalents each in aggregate, yielding total effective parallelism of roughly $2 + 8 times 0.16 = 3.3$ P-core equivalents. The observed speedup of 6.6 #sym.times exceeds this back-of-envelope estimate because the E-cores do contribute real throughput at their own IPC (1.28 at 12T, actually higher than the P-cores at 12T due to the P-core IPC penalty from cache pressure), just at lower clock frequency.

The instruction count increase from 1 thread to 12 threads (113 B #sym.arrow.r 132 B on P-cores alone) reflects OpenMP runtime overhead: fork/join, work-stealing bookkeeping, and reduction operations. These additional instructions contribute to the measured serial fraction without appearing as wall-clock time in the parallel region.


= Proposed Optimizations

*Tree-structured reduction.* The current flat `reduction(+:...)` accumulates partial sums in O(N) serial work at join. Replacing it with a log-depth hierarchical combine, either via `omp_get_thread_num()`-based binary fan-in or OpenMP's `task` directive, would reduce reduction overhead to O(log N) and directly address the weak-scaling failure. At 12 threads this would reduce the 4-level binary tree to 4 sequential operations instead of 12, and the improvement would compound as thread count grows. This is the single change most likely to recover Gustafson efficiency.

*Structure-of-Arrays (SoA) hand state.* Current hand representation is array-of-structs: each `Hand` object carries rank, suit, and value fields interleaved in memory. Converting to SoA would pack all rank values contiguously, allowing per-hand blackjack logic to fetch a full cache line of useful data rather than a mix of related and unrelated fields. This would reduce the effective miss penalty and also enable auto-vectorization of hand evaluation loops. Combined with the 71% miss rate observed in `perf stat`, this should reduce the effective miss penalty and may also improve IPC by reducing the instruction-window pressure that currently hides the stalls.

*Batched RNG.* The Mersenne Twister is invoked per card draw. Batching draws in chunks of 64 or 256 per shoe reshuffle would reduce function call overhead in the hot path and is likely to enable compiler vectorization of the generation loop, since MT's internal state update is data-parallel across the 624-word state array. An alternative is to replace MT with a lighter generator such as xorshift128+ or PCG, which generate 64 bits of randomness in 2 #sym.dash.en 4 instructions versus MT's heavier state advancement.

*P-core thread affinity pinning.* On the i7-1365U, binding all OpenMP threads to the two P-cores and their hyperthreads (or at minimum, weighting game dispatch toward P-core threads) would raise effective CPU utilization from 2.25 toward 4.0, improving both strong and weak scaling numbers without any algorithmic change. This can be implemented via `GOMP_CPU_AFFINITY` or `hwloc` bindings. The measured CPUs-utilized of 2.25 suggests this alone could nearly double the effective thread utilization without touching a line of simulation code.


= Conclusions

The original proposal listed five design refinements: privatization with thread-local RNG, lookup-table strategy decisions, SoA data layout, hierarchical reduction, and profiler-guided refinement. Of these, privatization is fully implemented via the deterministic seeding scheme, and lookup-table decisions are implemented implicitly through `switch`-based helpers that the compiler converts to jump tables. SoA layout and hierarchical reduction remain as future work. The proposed comparison against an existing open-source blackjack simulator was not completed.

The parallel blackjack simulator achieves both of its goals cleanly. On the blackjack side, only Basic (+0.09 #sym.cent) and Hi-Lo (+0.45 #sym.cent) return positive EV under Vegas Strip rules; every heuristic strategy yields to the house. The EV hierarchy is fully explainable: each step away from Basic removes a decision that correctly exploits the remaining shoe composition, and each removal adds expected loss proportional to the frequency and value of the missed opportunity.

On the parallel computing side, the outer-loop decomposition with per-game private state and a single OpenMP reduction is the right architectural choice. It delivers near-linear scaling up to the P-core count (98.8% efficiency at 2T on the i7), then plateaus as E-cores absorb remaining threads at lower effective IPC. The measured serial fraction of 7.4% on the i7 versus 5.0% on the M4 shows that Amdahl's $f$ depends on the hardware, not just the code: the same binary, on a homogeneous platform, exhibits a lower apparent serial fraction because all threads contribute equally to measured throughput.

The weak scaling result is the most disappointing number in the report: wall time roughly doubles as we go from 1 to 12 threads with proportional work, and the flat reduction is the main culprit. Fixing that, along with pinning threads to P-cores and switching to an SoA hand layout, should get strong scaling considerably closer to the 13.5 #sym.times Amdahl ceiling. The CPUs-utilized figure of 2.25 out of 12 is basically the hardware telling us that until we deal with the E-core scheduling and the reduction cost, adding more threads won't help much.

#v(1em)
#line(length: 100%, stroke: 0.4pt)
#v(0.5em)
#text(size: 9pt, fill: luma(80))[
  _Primary benchmark host: Intel Core i7-1365U (2P+8E, 12 threads), Void Linux 6.12, clang++ -O3 -march=native. \
  Cross-platform host: Apple M4, 10 P-cores, macOS Sequoia. Workload: 50,000 games #sym.times 100 rounds unless noted. \
  Build: C++23, CMake 3.20+, OpenMP via LLVM runtime. \
  CSI 4650 Parallel Computing, Oakland University, April 2026_
]
