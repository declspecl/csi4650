#include <blackjack/game/ruleset.hpp>
#include <blackjack/sim/run_report.hpp>

#include <iomanip>
#include <ostream>

namespace blackjack::sim {
    namespace {

        void write_ruleset_json(std::ostream& out, const game::Ruleset& rs) {
            out << "\"blackjack_payout_num\":" << static_cast<int>(rs.blackjack_payout_num)
                << ",\"blackjack_payout_denom\":" << static_cast<int>(rs.blackjack_payout_denom)
                << ",\"dealer_hits_soft_17\":" << (rs.dealer_hits_soft_17 ? "true" : "false")
                << ",\"surrender_allowed\":" << (rs.surrender_allowed ? "true" : "false")
                << ",\"double_after_split_allowed\":"
                << (rs.double_after_split_allowed ? "true" : "false")
                << ",\"max_split_hands\":" << static_cast<int>(rs.max_split_hands);
        }

        void write_betting_json(std::ostream& out, const game::BettingConfig& b) {
            out << "\"min_bet_cents\":" << b.get_min_bet()
                << ",\"max_bet_cents\":" << b.get_max_bet()
                << ",\"initial_bankroll_cents\":" << b.get_initial_bankroll();
        }

        void write_results_json(std::ostream& out, const game::GameStatistics& stats) {
            out << std::setprecision(17);
            out << "\"hands_played\":" << stats.get_hands_played()
                << ",\"starting_bankroll_cents\":" << stats.get_starting_bankroll()
                << ",\"ending_bankroll_cents\":" << stats.get_ending_bankroll()
                << ",\"bankroll_delta\":" << stats.get_bankroll_delta()
                << ",\"ev_per_hand\":" << stats.get_expected_value_per_hand();
        }

        void write_run_block_json(std::ostream& out, const SimRunConfig& cfg) {
            const game::Ruleset rules = game::Ruleset::default_vegas();
            out << "\"schema_version\":" << JSON_SCHEMA_VERSION
                << ",\"run\":{"
                << "\"base_seed\":" << cfg.base_seed
                << ",\"seed_scheme\":\"base_plus_game_times_rounds_plus_round\""
                << ",\"strategy\":\"" << strategy_name(cfg.strategy) << "\""
                << ",\"games\":" << cfg.game_count
                << ",\"rounds_per_game\":" << cfg.rounds_per_game
                << ",\"threads\":" << cfg.threads
                << ",\"betting\":{";
            write_betting_json(out, cfg.betting);
            out << "},\"ruleset\":{";
            write_ruleset_json(out, rules);
            out << "},\"ruleset_name\":\"default_vegas\""
                << "}";
        }
    }

    void write_text_report(
        std::ostream& out,
        const SimRunConfig& cfg,
        const game::GameStatistics& stats,
        double wall_time_ms
    ) {
        const game::Ruleset rules = game::Ruleset::default_vegas();
        out << "[run]\n";
        out << "schema_version:      " << JSON_SCHEMA_VERSION << "\n";
        out << "base_seed:           " << cfg.base_seed << "\n";
        out << "seed_scheme:         base_plus_game_times_rounds_plus_round\n";
        out << "strategy:            " << strategy_name(cfg.strategy) << "\n";
        out << "games:               " << cfg.game_count << "\n";
        out << "rounds_per_game:     " << cfg.rounds_per_game << "\n";
        out << "threads:             " << cfg.threads << "\n";
        out << "min_bet_cents:       " << cfg.betting.get_min_bet() << "\n";
        out << "max_bet_cents:       " << cfg.betting.get_max_bet() << "\n";
        out << "bankroll_cents:      " << cfg.betting.get_initial_bankroll() << "\n";
        out << "ruleset:             default_vegas "
            << "(bj " << static_cast<int>(rules.blackjack_payout_num) << "/"
            << static_cast<int>(rules.blackjack_payout_denom) << ", h17 "
            << (rules.dealer_hits_soft_17 ? "yes" : "no") << ")\n";
        out << "\n[results]\n";
        out << "hands_played:        " << stats.get_hands_played() << "\n";
        out << "starting_bankroll:   " << stats.get_starting_bankroll() << "\n";
        out << "ending_bankroll:     " << stats.get_ending_bankroll() << "\n";
        out << "bankroll_delta:      " << stats.get_bankroll_delta() << "\n";
        out << std::setprecision(17);
        out << "ev_per_hand:         " << stats.get_expected_value_per_hand() << "\n";
        out << std::setprecision(6);
        out << "wall_time_ms:        " << wall_time_ms << "\n";
    }

    void write_json_report(
        std::ostream& out,
        const SimRunConfig& cfg,
        const game::GameStatistics& stats
    ) {
        out << '{';
        write_run_block_json(out, cfg);
        out << ",\"results\":{";
        write_results_json(out, stats);
        out << "}}\n";
    }

    void write_json_benchmark_report(
        std::ostream& out,
        const SimRunConfig& cfg,
        const game::GameStatistics& serial_stats,
        const game::GameStatistics& parallel_stats
    ) {
        out << '{';
        write_run_block_json(out, cfg);
        out << ",\"mode\":\"benchmark\""
            << ",\"serial\":{";
        write_results_json(out, serial_stats);
        out << "},\"parallel\":{"
            << "\"threads\":" << cfg.threads << ',';
        write_results_json(out, parallel_stats);
        out << "}}\n";
    }
}
