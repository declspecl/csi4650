#include <blackjack/game/game.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/player/strategy/bearish.hpp>
#include <blackjack/player/strategy/bullish.hpp>

#include <gtest/gtest.h>

#include <memory>

using blackjack::game::BettingConfig;
using blackjack::game::Game;
using blackjack::player::strategy::BearishStrategy;
using blackjack::player::strategy::BullishStrategy;

static Game make_game() {
    BettingConfig config;
    Game game(config);
    game.initialize_round();
    return game;
}

TEST(RoundTest, DealerAlwaysEndsAtSeventeenOrBust) {
    Game game = make_game();
    game.play_round(42);

    const auto& dealer_hand = game.get_dealer().get_hand(0);
    EXPECT_TRUE(dealer_hand.get_value() >= 17 || dealer_hand.is_bust());
}

TEST(RoundTest, AllPlayersReceiveTwoCardsAfterDeal) {
    Game game = make_game();
    game.play_round(42);

    for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
        EXPECT_GE(game.get_player(i).get_hand(0).card_count(), 2u)
            << "Player " << static_cast<int>(i) << " has fewer than 2 cards";
    }
}

TEST(RoundTest, DealerReceivesTwoCardsAfterDeal) {
    Game game = make_game();
    game.play_round(42);

    EXPECT_GE(game.get_dealer().get_hand(0).card_count(), 2u);
}

TEST(RoundTest, HandsPlayedCountEqualsPlayerCount) {
    Game game = make_game();
    game.play_round(42);

    EXPECT_EQ(game.aggregate_statistics().get_hands_played(), Game::MAX_NON_DEALER_PLAYERS);
}

TEST(RoundTest, HandsPlayedAccumulatesAcrossRounds) {
    Game game = make_game();
    game.play_round(42);
    game.play_round(43);

    EXPECT_EQ(game.aggregate_statistics().get_hands_played(), 2 * Game::MAX_NON_DEALER_PLAYERS);
}

TEST(RoundTest, BearishPlayerNeverStandsBelow12) {
    Game game = make_game();

    for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
        game.get_player(i).set_strategy(std::make_unique<BearishStrategy>());
    }

    game.play_round(42);

    for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
        const auto& hand = game.get_player(i).get_hand(0);
        EXPECT_TRUE(hand.get_value() >= 12 || hand.is_bust())
            << "Player " << static_cast<int>(i)
            << " stood below 12 with value " << static_cast<int>(hand.get_value());
    }
}

TEST(RoundTest, SameSeedProducesDeterministicBankrollDelta) {
    Game game_a = make_game();
    game_a.play_round(99);
    int64_t delta_a = game_a.aggregate_statistics().get_bankroll_delta();

    Game game_b = make_game();
    game_b.play_round(99);
    int64_t delta_b = game_b.aggregate_statistics().get_bankroll_delta();

    EXPECT_EQ(delta_a, delta_b);
}

TEST(RoundTest, DifferentSeedsMayProduceDifferentOutcomes) {
    // Run many seeds; at least one pair should differ (collision would be astronomically unlikely)
    bool found_difference = false;

    Game base = make_game();
    base.play_round(0);
    int64_t base_delta = base.aggregate_statistics().get_bankroll_delta();

    for (uint64_t seed = 1; seed <= 20; seed++) {
        Game g = make_game();
        g.play_round(seed);
        if (g.aggregate_statistics().get_bankroll_delta() != base_delta) {
            found_difference = true;
            break;
        }
    }

    EXPECT_TRUE(found_difference);
}
