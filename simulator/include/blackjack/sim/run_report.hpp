#ifndef BLACKJACK_SIM_RUN_REPORT_HPP
#define BLACKJACK_SIM_RUN_REPORT_HPP

#include <blackjack/game/game_statistics.hpp>
#include <blackjack/sim/run_config.hpp>

#include <iosfwd>

namespace blackjack::sim {

    constexpr int JSON_SCHEMA_VERSION = 1;

    void write_text_report(
        std::ostream& out,
        const SimRunConfig& cfg,
        const game::GameStatistics& stats,
        double wall_time_ms
    );

    /** Deterministic JSON on `out` (stdout for the binary; stderr carries observed timings). */
    void write_json_report(
        std::ostream& out,
        const SimRunConfig& cfg,
        const game::GameStatistics& stats
    );

    void write_json_benchmark_report(
        std::ostream& out,
        const SimRunConfig& cfg,
        const game::GameStatistics& serial_stats,
        const game::GameStatistics& parallel_stats
    );
}

#endif
