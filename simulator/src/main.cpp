#include <blackjack/game/game.hpp>
#include <blackjack/game/ruleset.hpp>
#include <blackjack/player/strategy/always_stand.hpp>
#include <blackjack/player/strategy/basic.hpp>
#include <blackjack/player/strategy/bearish.hpp>
#include <blackjack/player/strategy/bullish.hpp>
#include <blackjack/player/strategy/double_first.hpp>
#include <blackjack/player/strategy/hi_lo.hpp>
#include <blackjack/player/strategy/mimic_dealer.hpp>
#include <blackjack/player/strategy/strategy.hpp>
#include <blackjack/player/strategy/surrender_first.hpp>
#include <blackjack/sim/run_config.hpp>
#include <blackjack/sim/run_report.hpp>
#include <blackjack/sim/simulation.hpp>

#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <omp.h>

using blackjack::game::Game;
using blackjack::game::GameStatistics;
using blackjack::player::strategy::AlwaysStandStrategy;
using blackjack::player::strategy::BasicStrategy;
using blackjack::player::strategy::BearishStrategy;
using blackjack::player::strategy::BullishStrategy;
using blackjack::player::strategy::DoubleFirstStrategy;
using blackjack::player::strategy::HiLoStrategy;
using blackjack::player::strategy::MimicDealerStrategy;
using blackjack::player::strategy::PlayerStrategy;
using blackjack::player::strategy::SurrenderFirstStrategy;

namespace blackjack::sim {
    namespace {

        bool parse_strategy(std::string_view s, StrategyKind& out) noexcept {
            if (s == "basic") {
                out = StrategyKind::BASIC;
                return true;
            }
            if (s == "mimic-dealer") {
                out = StrategyKind::MIMIC_DEALER;
                return true;
            }
            if (s == "bearish") {
                out = StrategyKind::BEARISH;
                return true;
            }
            if (s == "bullish") {
                out = StrategyKind::BULLISH;
                return true;
            }
            if (s == "always-stand") {
                out = StrategyKind::ALWAYS_STAND;
                return true;
            }
            if (s == "surrender-first") {
                out = StrategyKind::SURRENDER_FIRST;
                return true;
            }
            if (s == "double-first") {
                out = StrategyKind::DOUBLE_FIRST;
                return true;
            }
            if (s == "hi-lo") {
                out = StrategyKind::HI_LO;
                return true;
            }
            return false;
        }

        std::unique_ptr<PlayerStrategy> make_strategy(
            StrategyKind kind,
            bool das_allowed,
            bool h17
        ) {
            switch (kind) {
                case StrategyKind::BASIC:
                    return std::make_unique<BasicStrategy>(das_allowed);
                case StrategyKind::MIMIC_DEALER:
                    return std::make_unique<MimicDealerStrategy>(h17);
                case StrategyKind::BEARISH:
                    return std::make_unique<BearishStrategy>();
                case StrategyKind::BULLISH:
                    return std::make_unique<BullishStrategy>();
                case StrategyKind::ALWAYS_STAND:
                    return std::make_unique<AlwaysStandStrategy>();
                case StrategyKind::SURRENDER_FIRST:
                    return std::make_unique<SurrenderFirstStrategy>(das_allowed);
                case StrategyKind::DOUBLE_FIRST:
                    return std::make_unique<DoubleFirstStrategy>(das_allowed);
                case StrategyKind::HI_LO:
                    return std::make_unique<HiLoStrategy>(das_allowed);
            }
            return nullptr;
        }

        void seat_players_with_strategy(Game& game, StrategyKind kind) {
            bool das = game.get_ruleset().double_after_split_allowed;
            bool h17 = game.get_ruleset().dealer_hits_soft_17;
            for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
                game.get_player(i).set_strategy(make_strategy(kind, das, h17));
            }
        }

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

    std::string_view strategy_name(StrategyKind kind) noexcept {
        switch (kind) {
            case StrategyKind::BASIC:
                return "basic";
            case StrategyKind::MIMIC_DEALER:
                return "mimic-dealer";
            case StrategyKind::BEARISH:
                return "bearish";
            case StrategyKind::BULLISH:
                return "bullish";
            case StrategyKind::ALWAYS_STAND:
                return "always-stand";
            case StrategyKind::SURRENDER_FIRST:
                return "surrender-first";
            case StrategyKind::DOUBLE_FIRST:
                return "double-first";
            case StrategyKind::HI_LO:
                return "hi-lo";
        }
        return "unknown";
    }

    std::optional<SimRunConfig> try_parse_run_config(
        int argc,
        char* argv[],
        std::string& error
    ) noexcept {
        SimRunConfig cfg{};
        cfg.threads = omp_get_max_threads();

        uint32_t min_bet  = game::BettingConfig::DEFAULT_MIN_BET_CENTS;
        uint32_t max_bet  = game::BettingConfig::DEFAULT_MAX_BET_CENTS;
        uint32_t bankroll = game::BettingConfig::DEFAULT_INITIAL_BANKROLL_CENTS;

        for (int i = 1; i < argc; i++) {
            std::string_view arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                error = "help";
                return std::nullopt;
            }

            if (arg == "--benchmark") {
                cfg.benchmark = true;
                continue;
            }

            if (arg == "--format" && i + 1 < argc) {
                std::string_view name = argv[++i];
                if (name == "text") {
                    cfg.format = OutputFormat::TEXT;
                } else if (name == "json") {
                    cfg.format = OutputFormat::JSON;
                } else {
                    error = "Unknown --format (use text or json)";
                    return std::nullopt;
                }
                continue;
            }

            if (arg == "--strategy" && i + 1 < argc) {
                std::string_view name = argv[++i];
                if (!parse_strategy(name, cfg.strategy)) {
                    error = "Unknown strategy";
                    return std::nullopt;
                }
                continue;
            }

            if (arg == "--seed" && i + 1 < argc) {
                char* end              = nullptr;
                unsigned long long val = std::strtoull(argv[++i], &end, 10);
                if (end == argv[i] || *end != '\0') {
                    error = "Invalid --seed";
                    return std::nullopt;
                }
                cfg.base_seed = static_cast<uint64_t>(val);
                continue;
            }

            if ((arg == "--games" || arg == "--rounds" || arg == "--threads"
                 || arg == "--min-bet" || arg == "--max-bet" || arg == "--bankroll")
                && i + 1 < argc) {
                char* end            = nullptr;
                unsigned long val_ul = std::strtoul(argv[++i], &end, 10);
                if (end == argv[i] || *end != '\0') {
                    error = "Invalid numeric argument";
                    return std::nullopt;
                }
                if (arg == "--threads") {
                    if (val_ul < 1u || val_ul > static_cast<unsigned long>(INT_MAX)) {
                        error = "--threads must be >= 1";
                        return std::nullopt;
                    }
                    cfg.threads = static_cast<int>(val_ul);
                } else {
                    if (val_ul > UINT32_MAX) {
                        error = "Numeric argument too large";
                        return std::nullopt;
                    }
                    const auto val = static_cast<uint32_t>(val_ul);
                    if (arg == "--games") {
                        cfg.game_count = val;
                    } else if (arg == "--rounds") {
                        cfg.rounds_per_game = val;
                    } else if (arg == "--min-bet") {
                        min_bet = val;
                    } else if (arg == "--max-bet") {
                        max_bet = val;
                    } else if (arg == "--bankroll") {
                        bankroll = val;
                    }
                }
                continue;
            }

            error = "Unknown or incomplete argument";
            return std::nullopt;
        }

        if (cfg.game_count < 1u) {
            error = "--games must be >= 1";
            return std::nullopt;
        }
        if (cfg.rounds_per_game < 1u) {
            error = "--rounds must be >= 1";
            return std::nullopt;
        }

        cfg.betting = game::BettingConfig(min_bet, max_bet, bankroll);
        return cfg;
    }

    GameStatistics run_simulation(const SimRunConfig& cfg) {
        std::vector<Game> games;
        games.reserve(cfg.game_count);
        for (uint32_t g = 0; g < cfg.game_count; g++) {
            games.emplace_back(cfg.betting);
            games.back().initialize_round();
            seat_players_with_strategy(games.back(), cfg.strategy);
        }

        uint64_t total_hands    = 0;
        uint64_t total_starting = 0;
        uint64_t total_ending   = 0;

#pragma omp parallel for num_threads(cfg.threads) reduction(+ : total_hands, total_starting, total_ending)
        for (uint32_t g = 0; g < cfg.game_count; g++) {
            for (uint32_t r = 0; r < cfg.rounds_per_game; r++) {
                games[g].play_round(round_seed(cfg, g, r));
            }
            GameStatistics stats = games[g].aggregate_statistics();
            total_hands += stats.get_hands_played();
            total_starting += stats.get_starting_bankroll();
            total_ending += stats.get_ending_bankroll();
        }

        return GameStatistics(
            total_hands,
            total_starting,
            total_ending
        );
    }

    void print_usage(std::string_view program) noexcept {
        std::cerr
            << "Usage: " << program << " [options]\n"
            << "  --seed N          base RNG seed for round keys uint64 (default: 0)\n"
            << "  --games N         number of parallel games           (default: 1000)\n"
            << "  --rounds N        rounds per game                    (default: 100)\n"
            << "  --threads N       OMP thread count                   (default: max)\n"
            << "  --min-bet N       minimum bet in cents               (default: 100)\n"
            << "  --max-bet N       maximum bet in cents               (default: 10000)\n"
            << "  --bankroll N      initial bankroll in cents          (default: 100000)\n"
            << "  --strategy NAME   basic | mimic-dealer | bearish | bullish\n"
            << "                    always-stand | surrender-first | double-first | hi-lo\n"
            << "                    (default: basic)\n"
            << "  --format F        text | json (json: results on stdout; timings on stderr)\n"
            << "                    (default: text)\n"
            << "  --benchmark       run serial (1 thread) then parallel; compare wall time\n"
            << "  -h, --help        show this help\n";
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

#ifndef BLACKJACK_SIM_TESTING
int main(int argc, char* argv[]) {
    using blackjack::sim::OutputFormat;
    using blackjack::sim::SimRunConfig;
    using blackjack::sim::print_usage;
    using blackjack::sim::run_simulation;
    using blackjack::sim::try_parse_run_config;
    using blackjack::sim::write_json_benchmark_report;
    using blackjack::sim::write_json_report;
    using blackjack::sim::write_text_report;

    std::string err;
    std::optional<SimRunConfig> cfg_opt = try_parse_run_config(argc, argv, err);
    if (!cfg_opt.has_value()) {
        if (err == "help") {
            print_usage(argv[0]);
            return 0;
        }
        std::cerr << err << "\n";
        print_usage(argv[0]);
        return 1;
    }

    SimRunConfig cfg = *cfg_opt;

    using clock = std::chrono::steady_clock;
    using ms    = std::chrono::duration<double, std::milli>;

    if (cfg.benchmark) {
        SimRunConfig serial_cfg = cfg;
        serial_cfg.threads      = 1;

        auto   t0        = clock::now();
        auto   serial_s  = run_simulation(serial_cfg);
        double serial_ms = ms(clock::now() - t0).count();

        auto   t1          = clock::now();
        auto   parallel_s  = run_simulation(cfg);
        double parallel_ms = ms(clock::now() - t1).count();

        if (cfg.format == OutputFormat::JSON) {
            write_json_benchmark_report(std::cout, cfg, serial_s, parallel_s);
            std::clog << "observed serial_wall_time_ms=" << std::setprecision(17) << serial_ms
                      << " parallel_wall_time_ms=" << parallel_ms
                      << " speedup=" << ((parallel_ms > 0.0) ? (serial_ms / parallel_ms) : 0.0)
                      << "\n";
        } else {
            std::cout << "=== Serial (1 thread) ===\n";
            write_text_report(std::cout, serial_cfg, serial_s, serial_ms);
            std::cout << "\n=== Parallel (" << cfg.threads << " threads) ===\n";
            write_text_report(std::cout, cfg, parallel_s, parallel_ms);
            double speedup = (parallel_ms > 0.0) ? (serial_ms / parallel_ms) : 0.0;
            if (!std::isfinite(speedup)) {
                speedup = 0.0;
            }
            std::cout << "\n[observed]\n";
            std::cout << "speedup:             " << speedup << "x\n";
        }
        return 0;
    }

    auto   t0      = clock::now();
    auto   stats   = run_simulation(cfg);
    double wall_ms = ms(clock::now() - t0).count();

    if (cfg.format == OutputFormat::JSON) {
        write_json_report(std::cout, cfg, stats);
        std::clog << "observed wall_time_ms=" << std::setprecision(17) << wall_ms << "\n";
    } else {
        write_text_report(std::cout, cfg, stats, wall_ms);
    }

    return 0;
}
#endif
