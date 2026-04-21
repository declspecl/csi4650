#ifndef BLACKJACK_SIM_RUN_CONFIG_HPP
#define BLACKJACK_SIM_RUN_CONFIG_HPP

#include <blackjack/game/betting_config.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

/*
 * Reproducibility contract (schema_version 1):
 *
 * - Round RNG seeds are derived as:
 *     round_seed = base_seed + uint64_t(game_index) * rounds_per_game + round_index
 *   with 0 <= game_index < games and 0 <= round_index < rounds_per_game.
 *
 * - The simulation uses the default engine ruleset (Ruleset::default_vegas()) and
 *   default shoe geometry (Shoe uses six decks; see Shoe::MAX_SHOE_SIZE).
 *
 * - Integer statistics (hands played, bankroll cents) are deterministic for a given
 *   (base_seed, games, rounds_per_game, strategy, betting, threads) when using a
 *   fixed build of this binary. Floating-point EV is derived from those integers.
 *
 * - For `--format json`, wall-clock timings are written to stderr (`clog`) so stdout
 *   stays byte-stable for identical inputs. Those timings are not part of the game
 *   outcome contract.
 */

namespace blackjack::sim {

    enum class StrategyKind : uint8_t {
        BASIC,
        MIMIC_DEALER,
        BEARISH,
        BULLISH,
        ALWAYS_STAND,
        SURRENDER_FIRST,
        DOUBLE_FIRST,
        HI_LO,
    };

    enum class OutputFormat : uint8_t {
        TEXT,
        JSON,
    };

    struct SimRunConfig {
        uint64_t                    base_seed        = 0;
        uint32_t                    game_count       = 1000;
        uint32_t                    rounds_per_game  = 100;
        int                         threads          = 1; // parser overwrites with omp_get_max_threads()
        game::BettingConfig         betting          = game::BettingConfig{};
        StrategyKind                strategy         = StrategyKind::BASIC;
        OutputFormat                format           = OutputFormat::TEXT;
        bool                        benchmark        = false;
    };

    [[nodiscard]] std::string_view strategy_name(StrategyKind kind) noexcept;

    /** Parses argv; on failure returns nullopt and sets error message. Never exits. */
    [[nodiscard]] std::optional<SimRunConfig> try_parse_run_config(
        int argc,
        char* argv[],
        std::string& error
    ) noexcept;

    void print_usage(std::string_view program) noexcept;

    [[nodiscard]] constexpr uint64_t round_seed(
        const SimRunConfig& cfg,
        uint32_t game_index,
        uint32_t round_index
    ) noexcept {
        return cfg.base_seed
            + static_cast<uint64_t>(game_index) * cfg.rounds_per_game
            + static_cast<uint64_t>(round_index);
    }
}

#endif
