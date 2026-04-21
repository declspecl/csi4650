#include <blackjack/game/betting_config.hpp>
#include <blackjack/game/game.hpp>
#include <blackjack/player/strategy/always_stand.hpp>
#include <blackjack/player/strategy/basic.hpp>
#include <blackjack/player/strategy/bearish.hpp>
#include <blackjack/player/strategy/bullish.hpp>
#include <blackjack/player/strategy/double_first.hpp>
#include <blackjack/player/strategy/hi_lo.hpp>
#include <blackjack/player/strategy/mimic_dealer.hpp>
#include <blackjack/player/strategy/strategy.hpp>
#include <blackjack/player/strategy/surrender_first.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <omp.h>
#include <string_view>
#include <vector>

using blackjack::game::BettingConfig;
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

struct SimConfig {
    uint32_t      game_count      = 1000;
    uint32_t      rounds_per_game = 100;
    int           threads         = omp_get_max_threads();
    BettingConfig betting         = BettingConfig{};
    StrategyKind  strategy        = StrategyKind::BASIC;
};

static std::string_view strategy_name(StrategyKind kind) noexcept {
    switch (kind) {
        case StrategyKind::BASIC:          return "basic";
        case StrategyKind::MIMIC_DEALER:   return "mimic-dealer";
        case StrategyKind::BEARISH:        return "bearish";
        case StrategyKind::BULLISH:        return "bullish";
        case StrategyKind::ALWAYS_STAND:   return "always-stand";
        case StrategyKind::SURRENDER_FIRST: return "surrender-first";
        case StrategyKind::DOUBLE_FIRST:   return "double-first";
        case StrategyKind::HI_LO:          return "hi-lo";
    }
    return "unknown";
}

static std::unique_ptr<PlayerStrategy> make_strategy(StrategyKind kind, bool das_allowed, bool h17) {
    switch (kind) {
        case StrategyKind::BASIC:           return std::make_unique<BasicStrategy>(das_allowed);
        case StrategyKind::MIMIC_DEALER:    return std::make_unique<MimicDealerStrategy>(h17);
        case StrategyKind::BEARISH:         return std::make_unique<BearishStrategy>();
        case StrategyKind::BULLISH:         return std::make_unique<BullishStrategy>();
        case StrategyKind::ALWAYS_STAND:    return std::make_unique<AlwaysStandStrategy>();
        case StrategyKind::SURRENDER_FIRST: return std::make_unique<SurrenderFirstStrategy>(das_allowed);
        case StrategyKind::DOUBLE_FIRST:    return std::make_unique<DoubleFirstStrategy>(das_allowed);
        case StrategyKind::HI_LO:           return std::make_unique<HiLoStrategy>(das_allowed);
    }
    return nullptr;
}

static bool parse_strategy(std::string_view s, StrategyKind& out) {
    if (s == "basic")           { out = StrategyKind::BASIC;           return true; }
    if (s == "mimic-dealer")    { out = StrategyKind::MIMIC_DEALER;    return true; }
    if (s == "bearish")         { out = StrategyKind::BEARISH;         return true; }
    if (s == "bullish")         { out = StrategyKind::BULLISH;         return true; }
    if (s == "always-stand")    { out = StrategyKind::ALWAYS_STAND;    return true; }
    if (s == "surrender-first") { out = StrategyKind::SURRENDER_FIRST; return true; }
    if (s == "double-first")    { out = StrategyKind::DOUBLE_FIRST;    return true; }
    if (s == "hi-lo")           { out = StrategyKind::HI_LO;           return true; }
    return false;
}

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options]\n"
              << "  --games    N      number of parallel games     (default: 1000)\n"
              << "  --rounds   N      rounds per game              (default: 100)\n"
              << "  --threads  N      OMP thread count             (default: max)\n"
              << "  --min-bet  N      minimum bet in cents         (default: 100)\n"
              << "  --max-bet  N      maximum bet in cents         (default: 10000)\n"
              << "  --bankroll N      initial bankroll in cents    (default: 100000)\n"
              << "  --strategy NAME   player strategy (required to be explicit):\n"
              << "                      basic          - textbook basic strategy (baseline)\n"
              << "                      mimic-dealer   - play like the dealer (17 stand)\n"
              << "                      bearish        - hit until 12, then stand\n"
              << "                      bullish        - hit until 21\n"
              << "                      always-stand   - never hit (degenerate baseline)\n"
              << "                      surrender-first- surrender whenever legal, else basic\n"
              << "                      double-first   - double whenever legal, else basic\n"
              << "                      hi-lo          - Hi-Lo card counting with bet spread\n"
              << "                    (default: basic)\n";
}

static SimConfig parse_args(int argc, char* argv[]) {
    SimConfig cfg;
    uint32_t min_bet  = BettingConfig::DEFAULT_MIN_BET_CENTS;
    uint32_t max_bet  = BettingConfig::DEFAULT_MAX_BET_CENTS;
    uint32_t bankroll = BettingConfig::DEFAULT_INITIAL_BANKROLL_CENTS;

    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];

        if (arg == "--strategy" && i + 1 < argc) {
            std::string_view name = argv[++i];
            if (!parse_strategy(name, cfg.strategy)) {
                std::cerr << "Unknown strategy: " << name << "\n";
                print_usage(argv[0]);
                std::exit(1);
            }
            continue;
        }

        if ((arg == "--games" || arg == "--rounds" || arg == "--threads" ||
             arg == "--min-bet" || arg == "--max-bet" || arg == "--bankroll") && i + 1 < argc) {
            uint32_t val = static_cast<uint32_t>(std::atoi(argv[++i]));
            if      (arg == "--games")    cfg.game_count      = val;
            else if (arg == "--rounds")   cfg.rounds_per_game = val;
            else if (arg == "--threads")  cfg.threads         = static_cast<int>(val);
            else if (arg == "--min-bet")  min_bet             = val;
            else if (arg == "--max-bet")  max_bet             = val;
            else if (arg == "--bankroll") bankroll            = val;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            print_usage(argv[0]);
            std::exit(1);
        }
    }

    cfg.betting = BettingConfig(min_bet, max_bet, bankroll);
    return cfg;
}

static void seat_players_with_strategy(Game& game, StrategyKind kind) {
    bool das = game.get_ruleset().double_after_split_allowed;
    bool h17 = game.get_ruleset().dealer_hits_soft_17;
    for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
        game.get_player(i).set_strategy(make_strategy(kind, das, h17));
    }
}

static GameStatistics run_simulation(const SimConfig& cfg) {
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

    #pragma omp parallel for num_threads(cfg.threads) reduction(+:total_hands, total_starting, total_ending)
    for (uint32_t g = 0; g < cfg.game_count; g++) {
        for (uint32_t r = 0; r < cfg.rounds_per_game; r++) {
            games[g].play_round(static_cast<uint64_t>(g) * cfg.rounds_per_game + r);
        }
        GameStatistics stats = games[g].aggregate_statistics();
        total_hands    += stats.get_hands_played();
        total_starting += stats.get_starting_bankroll();
        total_ending   += stats.get_ending_bankroll();
    }

    return GameStatistics(total_hands, total_starting, total_ending);
}

static void print_results(const SimConfig& cfg, const GameStatistics& stats, double ms) {
    std::cout << "Strategy:          " << strategy_name(cfg.strategy) << "\n";
    std::cout << "Threads:           " << cfg.threads << "\n";
    std::cout << "Games:             " << cfg.game_count << "\n";
    std::cout << "Rounds per game:   " << cfg.rounds_per_game << "\n";
    std::cout << "Hands played:      " << stats.get_hands_played() << "\n";
    std::cout << "Starting bankroll: " << stats.get_starting_bankroll() << "\n";
    std::cout << "Ending bankroll:   " << stats.get_ending_bankroll() << "\n";
    std::cout << "Bankroll delta:    " << stats.get_bankroll_delta() << "\n";
    std::cout << "EV per hand:       " << stats.get_expected_value_per_hand() << "\n";
    std::cout << "Wall time:         " << ms << " ms\n";
}

int main(int argc, char* argv[]) {
    SimConfig cfg = parse_args(argc, argv);

    std::cout << "=== Single-threaded ===\n";
    SimConfig single = cfg;
    single.threads = 1;
    auto t0 = std::chrono::steady_clock::now();
    GameStatistics s1 = run_simulation(single);
    double t1 = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    print_results(single, s1, t1);

    std::cout << "\n=== Parallel (" << cfg.threads << " threads) ===\n";
    auto t2 = std::chrono::steady_clock::now();
    GameStatistics sN = run_simulation(cfg);
    double tN = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t2).count();
    print_results(cfg, sN, tN);

    std::cout << "\nSpeedup:           " << (t1 / tN) << "x\n";

    return 0;
}
