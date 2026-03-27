#include <blackjack/game/game.hpp>
#include <blackjack/player/player.hpp>
#include <blackjack/game/game_statistics.hpp>
#include <blackjack/game/betting_config.hpp>

#include <gtest/gtest.h>

using blackjack::game::BettingConfig;
using blackjack::game::Game;
using blackjack::game::GameStatistics;
using blackjack::player::Player;

TEST(StatisticsTest, SinglePlayerBankrollDelta) {
    BettingConfig config(100, 10000, 10000);
    Game game(config);

    game.initialize_round();

    Player& player = game.get_player(0);

    GameStatistics stats = game.aggregate_statistics();
    EXPECT_EQ(stats.get_starting_bankroll(), 70000);
    EXPECT_EQ(stats.get_hands_played(), 0);

    player.add_to_bankroll(2000);
    stats = game.aggregate_statistics();
    EXPECT_EQ(stats.get_hands_played(), 0);

    game.resolve_hand(player, 0);

    stats = game.aggregate_statistics();

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

    player.deduct_from_bankroll(2000);

    game.resolve_hand(player, 0);
    game.resolve_hand(player, 0);
    game.resolve_hand(player, 0);
    game.resolve_hand(player, 0);

    GameStatistics stats = game.aggregate_statistics();

    EXPECT_EQ(stats.get_hands_played(), 4);
    EXPECT_EQ(stats.get_bankroll_delta(), -2000);
    EXPECT_EQ(stats.get_expected_value_per_hand(), -500.0);
}

TEST(StatisticsTest, AggregateAcrossMultiplePlayers) {
    BettingConfig config(100, 10000, 10000);
    Game game(config);

    game.initialize_round();

    game.get_player(0).add_to_bankroll(1000);
    game.resolve_hand(game.get_player(0), 0);

    game.get_player(1).deduct_from_bankroll(500);
    game.resolve_hand(game.get_player(1), 0);

    game.get_player(2).add_to_bankroll(1500);
    game.resolve_hand(game.get_player(2), 0);

    GameStatistics total = game.aggregate_statistics();

    EXPECT_EQ(total.get_hands_played(), 3);
    EXPECT_EQ(total.get_starting_bankroll(), 70000);
    EXPECT_EQ(total.get_ending_bankroll(), 72000);
    EXPECT_EQ(total.get_bankroll_delta(), 2000);
}
