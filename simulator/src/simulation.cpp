#include <blackjack/game/game.hpp>
#include <blackjack/player/strategy/always_stand.hpp>
#include <blackjack/player/strategy/basic.hpp>
#include <blackjack/player/strategy/bearish.hpp>
#include <blackjack/player/strategy/bullish.hpp>
#include <blackjack/player/strategy/double_first.hpp>
#include <blackjack/player/strategy/hi_lo.hpp>
#include <blackjack/player/strategy/mimic_dealer.hpp>
#include <blackjack/player/strategy/surrender_first.hpp>
#include <blackjack/sim/simulation.hpp>

#include <memory>
#include <omp.h>
#include <vector>

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
            static_cast<uint32_t>(total_starting),
            static_cast<uint32_t>(total_ending)
        );
    }
}
