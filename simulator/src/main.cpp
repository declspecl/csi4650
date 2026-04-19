#include <blackjack/game/game.hpp>
#include <blackjack/game/betting_config.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <omp.h>
#include <string_view>
#include <vector>

using blackjack::game::BettingConfig;
using blackjack::game::Game;
using blackjack::game::GameStatistics;

struct SimConfig {
    uint32_t game_count      = 1000;
    uint32_t rounds_per_game = 100;
    int      threads         = omp_get_max_threads();
    BettingConfig betting    = BettingConfig{};
};

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options]\n"
              << "  --games    N   number of parallel games       (default: 1000)\n"
              << "  --rounds   N   rounds per game                (default: 100)\n"
              << "  --threads  N   OMP thread count               (default: max)\n"
              << "  --min-bet  N   minimum bet in cents           (default: 100)\n"
              << "  --max-bet  N   maximum bet in cents           (default: 10000)\n"
              << "  --bankroll N   initial bankroll in cents      (default: 100000)\n";
}

static SimConfig parse_args(int argc, char* argv[]) {
    SimConfig cfg;
    uint32_t min_bet  = BettingConfig::DEFAULT_MIN_BET_CENTS;
    uint32_t max_bet  = BettingConfig::DEFAULT_MAX_BET_CENTS;
    uint32_t bankroll = BettingConfig::DEFAULT_INITIAL_BANKROLL_CENTS;

    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];
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

static GameStatistics run_simulation(const SimConfig& cfg) {
    std::vector<Game> games;
    games.reserve(cfg.game_count);
    for (uint32_t g = 0; g < cfg.game_count; g++) {
        games.emplace_back(cfg.betting);
        games.back().initialize_round();
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

    return GameStatistics(
        total_hands,
        static_cast<uint32_t>(total_starting),
        static_cast<uint32_t>(total_ending)
    );
}

static void print_results(const SimConfig& cfg, const GameStatistics& stats, double ms) {
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
