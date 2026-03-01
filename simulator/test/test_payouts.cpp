#include "game.hpp"
#include "hand.hpp"
#include "betting_config.hpp"
#include "hand_outcome.hpp"
#include "hand_state.hpp"

#include <gtest/gtest.h>

TEST(PayoutTest, BlackjackWin) {
    BettingConfig config;
    Game game(config);

    // Test: $10 bet (1000 cents), blackjack win with 3:2 payout
    // Expected gross return: 1000 + (1000 * 3 / 2) = 2500 cents
    uint32_t payout = game.calculate_payout(HandOutcome::BLACKJACK_WIN, 1000);
    EXPECT_EQ(payout, 2500);
}

TEST(PayoutTest, RegularWin) {
    BettingConfig config;
    Game game(config);

    // Test: $10 bet (1000 cents), regular win with 1:1 payout
    // Expected gross return: 1000 + 1000 = 2000 cents
    uint32_t payout = game.calculate_payout(HandOutcome::REGULAR_WIN, 1000);
    EXPECT_EQ(payout, 2000);
}

TEST(PayoutTest, DealerBustWin) {
    BettingConfig config;
    Game game(config);

    // Test: $5 bet (500 cents), dealer bust win with 1:1 payout
    // Expected gross return: 500 + 500 = 1000 cents
    uint32_t payout = game.calculate_payout(HandOutcome::DEALER_BUST_WIN, 500);
    EXPECT_EQ(payout, 1000);
}

TEST(PayoutTest, Push) {
    BettingConfig config;
    Game game(config);

    // Test: $10 bet (1000 cents), push
    // Expected gross return: 1000 cents (bet returned)
    uint32_t payout = game.calculate_payout(HandOutcome::PUSH, 1000);
    EXPECT_EQ(payout, 1000);
}

TEST(PayoutTest, PlayerBustLoss) {
    BettingConfig config;
    Game game(config);

    // Test: $10 bet (1000 cents), player bust
    // Expected gross return: 0 cents (bet already deducted)
    uint32_t payout = game.calculate_payout(HandOutcome::PLAYER_BUST_LOSS, 1000);
    EXPECT_EQ(payout, 0);
}

TEST(PayoutTest, DealerWinLoss) {
    BettingConfig config;
    Game game(config);

    // Test: $10 bet (1000 cents), dealer wins
    // Expected gross return: 0 cents (bet already deducted)
    uint32_t payout = game.calculate_payout(HandOutcome::DEALER_WIN_LOSS, 1000);
    EXPECT_EQ(payout, 0);
}

TEST(PayoutTest, DealerBlackjackLoss) {
    BettingConfig config;
    Game game(config);

    // Test: $10 bet (1000 cents), dealer has blackjack
    // Expected gross return: 0 cents (bet already deducted)
    uint32_t payout = game.calculate_payout(HandOutcome::DEALER_BLACKJACK_LOSS, 1000);
    EXPECT_EQ(payout, 0);
}

// Split hand state machine tests
TEST(SplitHandTest, SplitHandWith21IsNotBlackjack) {
    BettingConfig config;
    Game game(config);

    // Create a hand with Ace + King = 21
    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::ACE));
    player_hand.add_card(Card(Suit::HEARTS, Rank::KING));

    // Create a dealer hand with 19
    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    // Verify hand is normally blackjack
    EXPECT_TRUE(player_hand.is_blackjack());

    // Mark hand as SPLIT (simulating it came from a split)
    player_hand.set_state(HandState::SPLIT);

    // Determine outcome - should be REGULAR_WIN not BLACKJACK_WIN
    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::REGULAR_WIN);
}

TEST(SplitHandTest, SplitHandWith21Pays1to1) {
    BettingConfig config;
    Game game(config);

    // Create a split hand with 21
    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::ACE));
    player_hand.add_card(Card(Suit::HEARTS, Rank::QUEEN));
    player_hand.set_state(HandState::SPLIT);

    // Create a dealer hand with 19
    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    // Determine outcome
    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::REGULAR_WIN);

    // Payout should be 1:1 (2000 cents total) not 3:2 (2500 cents total)
    uint32_t payout = game.calculate_payout(outcome, 1000);
    EXPECT_EQ(payout, 2000);
}

TEST(SplitHandTest, NormalBlackjackStillPays3to2) {
    BettingConfig config;
    Game game(config);

    // Create a normal (non-split) hand with blackjack
    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::ACE));
    player_hand.add_card(Card(Suit::HEARTS, Rank::KING));
    // Don't set SPLIT - hand is still PENDING

    // Create a dealer hand with 19
    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    // Determine outcome - should be BLACKJACK_WIN
    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::BLACKJACK_WIN);

    // Payout should be 3:2 (2500 cents total)
    uint32_t payout = game.calculate_payout(outcome, 1000);
    EXPECT_EQ(payout, 2500);
}

TEST(SplitHandTest, SplitHandCanBust) {
    BettingConfig config;
    Game game(config);

    // Create a split hand that busts
    Hand player_hand;
    player_hand.add_card(Card(Suit::SPADES, Rank::KING));
    player_hand.add_card(Card(Suit::HEARTS, Rank::QUEEN));
    player_hand.add_card(Card(Suit::DIAMONDS, Rank::FIVE));
    player_hand.set_state(HandState::SPLIT);

    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::KING));
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));

    // Determine outcome - should be PLAYER_BUST_LOSS
    HandOutcome outcome = game.determine_outcome(player_hand, dealer_hand);
    EXPECT_EQ(outcome, HandOutcome::PLAYER_BUST_LOSS);

    // Should get nothing back (bet already deducted)
    uint32_t payout = game.calculate_payout(outcome, 1000);
    EXPECT_EQ(payout, 0);
}
