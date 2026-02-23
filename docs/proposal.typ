#set text(font: "Times New Roman")
#set page(paper: "a4")
#set align(center + horizon)

= Group Project Proposal
== Parallel Blackjack Simulator

#v(0.25cm)

Gavin D'Hondt, Anton Sakhanovych

CSI 4650, Dr. Jiannan Tian

2026-02-19

#pagebreak()

#set align(top + left)
#set heading(numbering: "1.")
#set par(first-line-indent: 2em)

= Introduction

Blackjack simulation is a practical Monte Carlo workload that sits at the boundary between trivially parallel computation (many independent games) and execution challenges that stress parallel design: synchronization during aggregation, contention on shared RNG state, and reduction of statistics across threads.
This project proposes a Blackjack simulator implemented in C++ with OpenMP, designed to explore parallelization—decomposition, parallel patterns, and synchronization—as the primary focus, with profiling used to evaluate and justify our parallel design choices.

The goal is to design effective parallel decomposition and minimize synchronization overhead, then use profiling (runtime, hardware counters) to validate and explain the results.
The primary deliverable is a parallel, cross-platform Blackjack simulator with a comprehensive configuration system.
We will evaluate how different parallelization choices (hierarchical parallelism, privatization, reduction patterns) affect scalability, and run the simulator on multiple CPU platforms when possible.

== Problem Statement

This project aims to produce a multi-purpose solution to fit many use cases.
It is a known fact that for a single deck game:
- Without perfect basic strategy, the house edge is 2%
- With perfect basic strategy, the house edge is 0.5%
- With perfect card counting and betting, the player edge is expected to be between 0.5% and 1.5%

Players who do not know the optimal strategies are strictly expected to lose money over time.
Players who do know basic strategy and only suboptimal counting or betting strategies are expected to stay relatively even.

This project aims to definitely be able to answer what the optimal strategies are across various scenarios, and enable users to easily test their own strategies.

== Why This Project?

At first glance, a Blackjack simulator appears trivially parallel: each hand can be simulated independently.
However, Blackjack is limited in parallelism in a more subtle way: the most straightforward design is essentially a single parallel map (simulate hands) followed by a reduction (aggregate statistics).
Additional speedup is often capped by non-parallel and hardware-limited costs that dominate the hot path, such as random number generation, branch-heavy decision logic, and shared-memory effects during aggregation.

This gives the project several parallelization opportunities central to the course:
- Hierarchical parallelism: decompose work across both strategy/rule configurations and batches of hands
- Synchronization and privatization: use thread-local RNG streams and thread-local statistics buffers, then merge via a reduction to avoid atomics and false sharing
- Map-reduce pattern: embarrassingly parallel map (simulate hands) followed by a reduction (merge statistics)
- Supporting refinements: data layout and lookup tables to reduce per-hand overhead and improve scalability; profiling to validate that our parallel design achieves the expected benefits

#pagebreak()

= Related Work

Monte Carlo simulation of card games has a long history in both academic and industry settings.
Edward Thorp's foundational work on optimal Blackjack strategy and card counting in the 1960s established the mathematical framework that modern simulators rely on.
Julian Braun's simulations at IBM in the 1960s and 1970s produced the canonical basic strategy tables still in use today.
More recently, parallel Monte Carlo methods have been applied extensively to similar workloads: financial option pricing, particle physics, and other stochastic simulations.

Techniques relevant to this project include:
- Parallel reductions and map-reduce patterns for aggregating independent trial results without data races
- Thread-local storage and privatization to eliminate synchronization (atomic adds) in favor of local buffers with a final merge
- Loop-level optimizations such as structure-of-arrays (SOA) layouts and lookup tables to reduce branching and improve cache locality
- Fast parallel random number generators (e.g., xorshift, PCG, or library-specific streams) to avoid RNG contention
- Profiling (perf, DTrace, hardware counters) to evaluate parallel designs and attribute speedup to specific parallelization choices

We will survey existing Blackjack simulators and parallel Monte Carlo frameworks to identify baseline implementations and parallelization approaches.


= Design

Our design is parallelization-centric: we decompose the workload for parallelism first, then apply refinements to improve scalability. Profiling is used to evaluate each stage.

The parallelization strategy relies on hierarchical decomposition.
We parallelize over strategy and rule configurations in the outer loop, and over batches of hands in the inner loop, so that work is distributed across cores without unnecessary synchronization.
To avoid contention, each thread uses its own RNG stream and its own statistics buffer; threads accumulate locally and merge results in a final reduction phase.
This avoids atomic operations in the hot path and sidesteps false sharing.

Refinements support parallel scalability by keeping per-hand cost low.
We will use structure-of-arrays (SOA) and compact state to improve cache locality, lookup tables for strategy decisions (hit/stand/double/split) instead of branch-heavy logic, and batch processing to improve locality and enable vectorization within each thread.
These changes reduce the overhead each thread pays per hand, which helps parallel speedup scale.

Profiling will evaluate whether our parallel design achieves the expected benefits. The baseline is a single-threaded implementation.
We will measure runtime, throughput (hands/second), speedup, IPC, branch misprediction rate, and cache miss rate, using Linux perf, DTrace where available, and Apple Instruments for Apple Silicon.
Performance models (once covered in lecture) may help bound or explain observed speedups.

We plan to run the simulator on multiple platforms (e.g., x86_64 Linux and Apple Silicon) when possible and document platform-specific behavior.
Apple's in-house CPUs are known for wide, deep execution, which may affect how well our parallelization scales.


= Evaluation

We will evaluate our parallel design in stages, adding parallelism and refinements incrementally and using profiling to attribute gains to specific parallelization choices.

The baseline is a single-threaded implementation with correct semantics.
We may also compare against an existing open-source Blackjack simulator if a suitable reference exists.

We will apply the following stages incrementally and measure each in isolation:
1. Multi-threaded parallelization with OpenMP using naive shared-memory aggregation
2. Privatization with thread-local RNG and statistics buffers and a reduction merge
3. Lookup-table-based strategy decisions
4. Data layout changes (SOA, compact state)
5. Profiler-guided refinement to validate and explain parallel scalability

For each step we will report throughput, speedup over baseline, and relevant hardware metrics.
We will explain and justify observed speedups rather than emphasize a standalone number; if speedup is limited, we will analyze why (e.g., ILP vs TLP tradeoffs, cache coherence overhead) and document those insights.


= Insights

We expect to generate insights along the following lines:
- How parallelization choices (decomposition, privatization, reduction pattern) affect scalability on this workload
- Tradeoffs between thread-level parallelism and per-thread efficiency (e.g., ILP vs TLP, cache coherence overhead)
- Whether hardware metrics (IPC, cache behavior) help explain observed parallel speedup
- Platform-specific scaling behavior and implications for generalizability

These insights will be documented as we complete each stage of the parallel design.


= Conclusion

This project proposes a parallel Blackjack simulator with parallelism as the central design focus: decomposition, synchronization, and scalability.
Profiling will be used to evaluate and justify our parallel design choices.
We will use Dr. Tian's recommended structure—introduction, related work, design, evaluation, and insights—to report our findings.
