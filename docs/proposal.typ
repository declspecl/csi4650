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

Blackjack simulation is a practical Monte Carlo workload that sits at the boundary between trivially parallel computation in simulating many independent games and performance-limited execution (branch-heavy decision logic, frequent random number generation, and statistics aggregation).
This project proposes a high-performance Blackjack simulator implemented in C++ with OpenMP, designed to explore both parallelization and acceleration techniques for this simulation workload.
We plan to simulate large numbers of games across multiple game rule configurations, player strategies, including different card counting systems, betting systems, and apply profiling-guided optimization to understand and justify performance gains.

The goal of this project is not only to demonstrate speedup but to identify bottlenecks, apply targeted optimizations, and explain results using evidence (runtime breakdowns and hardware performance counters).
The primary deliverable of this project is a high-preformance, cross-platform Blackjack simulator with a comprehensive configuration system that enables users to easily determine optimal strategies and rules for different scenarios and player types.
Along the way, we will compare how different optimization techniques contribute to throughput and scalability, and we will evaluate the simulator across multiple CPU platforms when possible.

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

This results in the project having several opportunities to implement parallelization to improve performance:
- Hierarchical parallelism: parallelize across both strategy/rules configurations and batches of hands
- Contention elimination: use thread-local RNG streams and thread-local statistics buffers, then merge results at the end to avoid atomics and false sharing
- Acceleration of the hot loop: restructure data and control flow (SOA vs AOS, compact state, lookup tables, reduced branching) and explore batching to increase locality and enable compiler vectorization where possible
- Evidence-based optimization: use profiling tools and hardware counters (e.g., IPC, branch misses, cache misses) to attribute performance changes to specific bottlenecks and to justify observed speedups

#pagebreak()

= Related Work

#pagebreak()

= Design

#pagebreak()


#pagebreak()

= Insights

#pagebreak()

= Conclusion
