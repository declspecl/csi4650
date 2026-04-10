#include <blackjack/game/game.hpp>

#include <iostream>

int main() {
    constexpr uint64_t SEED = 42;
    constexpr uint32_t ROUND_COUNT = 100;

    blackjack::game::Game game;
    game.initialize_round();

    for (uint32_t i = 0; i < ROUND_COUNT; i++) {
        game.play_round(SEED + i);
    }

    blackjack::game::GameStatistics stats = game.aggregate_statistics();

    std::cout << "Rounds:           " << ROUND_COUNT << "\n";
    std::cout << "Hands played:     " << stats.get_hands_played() << "\n";
    std::cout << "Starting bankroll: " << stats.get_starting_bankroll() << "\n";
    std::cout << "Ending bankroll:  " << stats.get_ending_bankroll() << "\n";
    std::cout << "Bankroll delta:   " << stats.get_bankroll_delta() << "\n";
    std::cout << "EV per hand:      " << stats.get_expected_value_per_hand() << "\n";

    return 0;
}
