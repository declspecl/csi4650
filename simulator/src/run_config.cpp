#include <blackjack/sim/run_config.hpp>

#include <climits>
#include <cstdlib>
#include <iostream>
#include <omp.h>

namespace blackjack::sim {

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

    static bool parse_strategy(std::string_view s, StrategyKind& out) noexcept {
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
                char* end        = nullptr;
                unsigned long long v = std::strtoull(argv[++i], &end, 10);
                if (end == argv[i] || *end != '\0') {
                    error = "Invalid --seed";
                    return std::nullopt;
                }
                cfg.base_seed = static_cast<uint64_t>(v);
                continue;
            }

            if ((arg == "--games" || arg == "--rounds" || arg == "--threads" ||
                 arg == "--min-bet" || arg == "--max-bet" || arg == "--bankroll")
                && i + 1 < argc) {
                char* end              = nullptr;
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
}
