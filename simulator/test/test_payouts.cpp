#include <blackjack/game/game.hpp>
#include <blackjack/hand/hand.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/hand/hand_outcome.hpp>
#include <blackjack/hand/hand_origin.hpp>

#include <gtest/gtest.h>

using blackjack::card::Card;
using blackjack::card::Suit;
using blackjack::card::Rank;
using blackjack::game::BettingConfig;
using blackjack::game::Game;
using blackjack::hand::Hand;
using blackjack::hand::HandOrigin;
using blackjack::hand::HandOutcome;

TEST(PayoutTest, BlackjackWin) {
    BettingConfig config;
    Game game(config);

    uint32_t payout = game.calculate_payout(HandOutcome::BLACKJACK_WIN, 1000);
    EXPECT_EQ(payout, 2500);
}

TEST(PayoutTest, RegularWin) {
    BettingConfig config;
    Game game(config);

    uint32_t payout = game.calculate_payout(HandOutcome::REGULAR_WIN, 1000);
    EXPECT_EQ(payout, 2000);
}

TEST(PayoutTest, DealerBustWin) {
    BettingConfig config;
    Game game(config);

    uint32_t payout = game.calculate_payout(HandOutcome::DEALER_BUST_WIN, 500);
    EXPECT_EQ(payout, 1000);
}

TEST(PayoutTest, Push) {
    BettingConfig config;
    Game game(config);

    uint32_t payout = game.calculate_payout(HandOutcome::PUSH, 1000);
    EXPECT_EQ(payout, 1000);
}

TEST(PayoutTest, PlayerBustLoss) {
    BettingConfig config;
    Game game(config);

    uint32_t payout = game.calculate_payout(HandOutcome::PLAYER_BUST_LOSS, 1000);
    EXPECT_EQ(payout, 0);
}

TEST(PayoutTest, DealerWinLoss) {
    BettingConfig config;
    Game game(config);

    uint32_t payout = game.calculate_payout(HandOutcome::DEALER_WIN_LOSS, 1000);
    EXPECT_EQ(payout, 0);
}

TEST(PayoutTest, DealerBlackjackLoss) {
    BettingConfig config;
    Game game(config);

    uint32_t payout = game.calculate_payout(HandOutcome::DEALER_BLACKJACK_LOSS, 1000);
    EXPECT_EQ(payout, 0);
}

TEST(SplitHandTest, SplitHandWith21IsNotBlackjack) {
    BettingConfig config;
    Game game(config);

    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::ACE));
    player_hand.add_card(Card(Suit::HEARTS, Rank::KING));

    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    EXPECT_TRUE(player_hand.is_blackjack());

    player_hand.set_origin(HandOrigin::SPLIT);

    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::REGULAR_WIN);
}

TEST(SplitHandTest, SplitHandWith21Pays1to1) {
    BettingConfig config;
    Game game(config);

    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::ACE));
    player_hand.add_card(Card(Suit::HEARTS, Rank::QUEEN));
    player_hand.set_origin(HandOrigin::SPLIT);

    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::REGULAR_WIN);

    uint32_t payout = game.calculate_payout(outcome, 1000);
    EXPECT_EQ(payout, 2000);
}

TEST(SplitHandTest, NormalBlackjackStillPays3to2) {
    BettingConfig config;
    Game game(config);

    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::ACE));
    player_hand.add_card(Card(Suit::HEARTS, Rank::KING));

    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::BLACKJACK_WIN);

    uint32_t payout = game.calculate_payout(outcome, 1000);
    EXPECT_EQ(payout, 2500);
}

TEST(SplitHandTest, SplitHandCanBust) {
    BettingConfig config;
    Game game(config);

    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::KING));
    player_hand.add_card(Card(Suit::HEARTS, Rank::QUEEN));
    player_hand.add_card(Card(Suit::DIAMONDS, Rank::FIVE));
    player_hand.set_origin(HandOrigin::SPLIT);

    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::PLAYER_BUST_LOSS);

    uint32_t payout = game.calculate_payout(outcome, 1000);
    EXPECT_EQ(payout, 0);
}
