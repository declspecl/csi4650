#include "game.hpp"
#include "player.hpp"
#include "game_statistics.hpp"
#include "betting_config.hpp"

#include <gtest/gtest.h>

TEST(StatisticsTest, SinglePlayerBankrollDelta) {
    BettingConfig config(100, 10000, 10000); // min $1, max $100, starting $100
    Game game(config);

    // Initialize game (initializes all 7 players, each with $100)
    game.initialize_round();

    Player& player = game.get_player(0);

    // Verify initial state: starting bankroll is tracked (7 players * $100)
    GameStatistics stats = game.aggregate_statistics();
    EXPECT_EQ(stats.get_starting_bankroll(), 70000); // 7 * 10000
    EXPECT_EQ(stats.get_hands_played(), 0);

    // Simulate player 0 winning a hand (+$20)
    player.add_to_bankroll(2000);
    stats = game.aggregate_statistics();
    EXPECT_EQ(stats.get_hands_played(), 0); // Not incremented yet

    // Now simulate hand being resolved (increments counter)
    game.resolve_hand(player, 0);

    // Finalize to calculate ending bankroll
    game.finalize_player_statistics();
    stats = game.aggregate_statistics();

    // Check: 1 hand played
    // Ending = 12000 (player 0) + 60000 (other 6 players) = 72000
    // Delta = +2000
    EXPECT_EQ(stats.get_hands_played(), 1);
    EXPECT_EQ(stats.get_ending_bankroll(), 72000);
    EXPECT_EQ(stats.get_bankroll_delta(), 2000);
    EXPECT_EQ(stats.get_expected_value_per_hand(), 2000.0);
}

TEST(StatisticsTest, MultipleHandsTracking) {
    BettingConfig config(100, 10000, 10000);
    Game game(config);

    game.initialize_round();
    Player& player = game.get_player(0);

    // Simulate 4 hands being played (net loss of $20)
    player.deduct_from_bankroll(2000);

    // Manually increment hands (simulating 4 hand resolutions)
    game.resolve_hand(player, 0);
    game.resolve_hand(player, 0);
    game.resolve_hand(player, 0);
    game.resolve_hand(player, 0);

    game.finalize_player_statistics();
    GameStatistics stats = game.aggregate_statistics();

    // Delta: 8000 - 10000 = -2000
    // EV per hand: -2000 / 4 = -500 cents = -$5
    EXPECT_EQ(stats.get_hands_played(), 4);
    EXPECT_EQ(stats.get_bankroll_delta(), -2000);
    EXPECT_EQ(stats.get_expected_value_per_hand(), -500.0);
}

TEST(StatisticsTest, AggregateAcrossMultiplePlayers) {
    BettingConfig config(100, 10000, 10000);
    Game game(config);

    // Initialize round (all 7 players start with $100 each)
    game.initialize_round();

    // Simulate different outcomes for first 3 players
    // Player 0: wins $10
    game.get_player(0).add_to_bankroll(1000);
    game.resolve_hand(game.get_player(0), 0);

    // Player 1: loses $5
    game.get_player(1).deduct_from_bankroll(500);
    game.resolve_hand(game.get_player(1), 0);

    // Player 2: wins $15
    game.get_player(2).add_to_bankroll(1500);
    game.resolve_hand(game.get_player(2), 0);

    // Players 3-6 don't play (bankrolls unchanged)

    // Finalize and get statistics
    game.finalize_player_statistics();
    GameStatistics total = game.aggregate_statistics();

    // Total: 3 hands played
    // Total starting: 7 * 10000 = 70000 cents
    // Total ending: (11000 + 9500 + 11500 + 10000*4) = 72000 cents
    // Total delta: +2000 cents (+$20)
    EXPECT_EQ(total.get_hands_played(), 3);
    EXPECT_EQ(total.get_starting_bankroll(), 70000);
    EXPECT_EQ(total.get_ending_bankroll(), 72000);
    EXPECT_EQ(total.get_bankroll_delta(), 2000);
}
