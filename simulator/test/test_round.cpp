#include <blackjack/game/game.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/player/strategy/bearish.hpp>
#include <blackjack/player/strategy/bullish.hpp>
#include <blackjack/player/strategy/strategy.hpp>
#include <blackjack/card/card.hpp>

#include <gtest/gtest.h>

#include <memory>

using blackjack::card::Card;
using blackjack::card::Rank;
using blackjack::card::Suit;
using blackjack::game::ActionApplicationResult;
using blackjack::game::BettingConfig;
using blackjack::game::Game;
using blackjack::hand::Hand;
using blackjack::player::strategy::BearishStrategy;
using blackjack::player::strategy::BullishStrategy;
using blackjack::player::strategy::Decision;

static Game make_game() {
    BettingConfig config;
    Game game(config);
    game.initialize_round();
    return game;
}

static void set_player_hand(
    Game& game,
    uint8_t player_index,
    Card first,
    Card second,
    uint32_t bet
) {
    auto& player = game.get_player(player_index);
    player.clear_hand(0);
    player.set_active_hand_count(1);
    player.add_card_to_hand(0, first);
    player.add_card_to_hand(0, second);
    player.set_hand_bet(0, bet);
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

TEST(RoundTest, ApplyDecisionHitDrawsOneCard) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::FOUR),
        Card(Suit::HEARTS, Rank::FIVE),
        100
    );

    EXPECT_EQ(game.get_player(0).get_hand(0).card_count(), 2u);

    ActionApplicationResult result = game.apply_decision(0, 0, Decision::HIT);

    EXPECT_EQ(result, ActionApplicationResult::APPLIED_CONTINUE);
    EXPECT_EQ(game.get_player(0).get_hand(0).card_count(), 3u);
}

TEST(RoundTest, ApplyDecisionStandEndsTurnExplicitly) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    EXPECT_EQ(game.apply_decision(0, 0, Decision::STAND), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::HIT), ActionApplicationResult::ILLEGAL_ACTION);
}

TEST(RoundTest, EligibleDoubleIsExplicitlyUnsupported) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::FIVE),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    EXPECT_EQ(game.apply_decision(0, 0, Decision::DOUBLE), ActionApplicationResult::UNSUPPORTED_ACTION);
}

TEST(RoundTest, EligibleSplitIsExplicitlyUnsupported) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );

    EXPECT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::UNSUPPORTED_ACTION);
}

TEST(RoundTest, EligibleSurrenderIsExplicitlyUnsupported) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    EXPECT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::UNSUPPORTED_ACTION);
}

TEST(RoundTest, ApplyDecisionOnInactiveHandIndexIsIllegal) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    // active_hand_count is 1, so hand_index 1..MAX-1 are all inactive.
    EXPECT_EQ(game.apply_decision(0, 1, Decision::HIT), ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 1, Decision::STAND), ActionApplicationResult::ILLEGAL_ACTION);
}

TEST(RoundTest, HitUntilBustCompletesTurnAndRejectsFurtherActions) {
    Game game = make_game();
    // Force a guaranteed-bust scenario: 10 + 6 + extra hits until value > 21.
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    ActionApplicationResult result = ActionApplicationResult::APPLIED_CONTINUE;
    for (int i = 0; i < Hand::MAX_HAND_SIZE && result == ActionApplicationResult::APPLIED_CONTINUE; i++) {
        result = game.apply_decision(0, 0, Decision::HIT);
    }

    // Whichever card busts the hand, the final HIT must report turn-complete.
    EXPECT_EQ(result, ActionApplicationResult::APPLIED_TURN_COMPLETE);
    EXPECT_TRUE(game.get_player(0).get_hand(0).is_bust());

    // Subsequent actions on a completed hand are illegal.
    EXPECT_EQ(game.apply_decision(0, 0, Decision::HIT), ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::STAND), ActionApplicationResult::ILLEGAL_ACTION);
}
