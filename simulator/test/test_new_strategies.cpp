#include <blackjack/card/card.hpp>
#include <blackjack/card/rank.hpp>
#include <blackjack/card/suit.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/hand/hand.hpp>
#include <blackjack/player/strategy/always_stand.hpp>
#include <blackjack/player/strategy/double_first.hpp>
#include <blackjack/player/strategy/hi_lo.hpp>
#include <blackjack/player/strategy/surrender_first.hpp>
#include <blackjack/player/strategy/strategy.hpp>

#include <gtest/gtest.h>

using blackjack::card::Card;
using blackjack::card::Rank;
using blackjack::card::Suit;
using blackjack::game::BettingConfig;
using blackjack::hand::Hand;
using blackjack::player::strategy::AlwaysStandStrategy;
using blackjack::player::strategy::Decision;
using blackjack::player::strategy::DoubleFirstStrategy;
using blackjack::player::strategy::GameContext;
using blackjack::player::strategy::HiLoStrategy;
using blackjack::player::strategy::LegalActions;
using blackjack::player::strategy::SurrenderFirstStrategy;

namespace {
    Hand make_hand(Card a, Card b) {
        Hand h;
        h.add_card(a);
        h.add_card(b);
        return h;
    }

    GameContext ctx(const Hand& hand, Card upcard, LegalActions legal,
                    int16_t running_count = 0, uint16_t cards_remaining = 312) {
        return GameContext(hand, upcard, legal, running_count, cards_remaining);
    }

    constexpr LegalActions all_legal()      { return {true,  true,  true};  }
    constexpr LegalActions only_hit_stand() { return {false, false, false}; }
    constexpr LegalActions can_double()     { return {true,  false, false}; }
    constexpr LegalActions can_surrender()  { return {false, false, true};  }
}

// ── AlwaysStand ──────────────────────────────────────────────────────────────

TEST(AlwaysStandTest, StandsOnLowHand) {
    AlwaysStandStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TWO), Card(Suit::HEARTS, Rank::THREE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::STAND);
}

TEST(AlwaysStandTest, StandsEvenWhenDoubleOrSurrenderIsLegal) {
    AlwaysStandStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), all_legal())), Decision::STAND);
}

TEST(AlwaysStandTest, StandsOnBlackjack) {
    AlwaysStandStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::ACE), Card(Suit::HEARTS, Rank::KING));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), only_hit_stand())), Decision::STAND);
}

// ── SurrenderFirst ───────────────────────────────────────────────────────────

TEST(SurrenderFirstTest, SurrendersWhenLegal) {
    SurrenderFirstStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), can_surrender())), Decision::SURRENDER);
}

TEST(SurrenderFirstTest, FallsBackToBasicWhenSurrenderNotAllowed) {
    SurrenderFirstStrategy s;
    // Hard 16 vs 10 — basic says HIT when surrender not available
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));
    Decision d = s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), only_hit_stand()));
    EXPECT_EQ(d, Decision::HIT);
}

TEST(SurrenderFirstTest, FallsBackToBasicStandOnGoodHand) {
    SurrenderFirstStrategy s;
    // Hard 18 vs 6 — basic says STAND; surrender not legal
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::EIGHT));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), only_hit_stand())), Decision::STAND);
}

// ── DoubleFirst ──────────────────────────────────────────────────────────────

TEST(DoubleFirstTest, DoublesWhenLegal) {
    DoubleFirstStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), can_double())), Decision::DOUBLE);
}

TEST(DoubleFirstTest, FallsBackToBasicWhenDoubleNotAllowed) {
    DoubleFirstStrategy s;
    // Hard 11 vs 6, no double — basic says HIT
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), only_hit_stand())), Decision::HIT);
}

TEST(DoubleFirstTest, FallsBackToBasicStandOnStrongHand) {
    DoubleFirstStrategy s;
    // Hard 20, no double — basic says STAND
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::TEN));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), only_hit_stand())), Decision::STAND);
}

// ── HiLo ─────────────────────────────────────────────────────────────────────

TEST(HiLoTest, FollowsBasicAtZeroCount) {
    HiLoStrategy s;
    // Hard 11 vs 6 — basic says DOUBLE when available
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), can_double(), 0, 312)), Decision::DOUBLE);
}

TEST(HiLoTest, StandsHard16VsDealerTenAtZeroCount) {
    HiLoStrategy s;
    // TC=0: deviation says STAND (basic says HIT)
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));
    // running=0, remaining=312 → true count = 0
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), only_hit_stand(), 0, 312)), Decision::STAND);
}

TEST(HiLoTest, HitsHard16VsDealerTenAtNegativeCount) {
    HiLoStrategy s;
    // TC = -3 / (156/52) = -1: no deviation → basic says HIT
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), only_hit_stand(), -3, 156)), Decision::HIT);
}

TEST(HiLoTest, StandsHard12VsThreeAtHighCount) {
    HiLoStrategy s;
    // TC = +6 / (156/52) = +2: deviation says STAND (basic says HIT)
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SEVEN), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::THREE), only_hit_stand(), 6, 156)), Decision::STAND);
}

TEST(HiLoTest, HitsHard12VsThreeAtLowCount) {
    HiLoStrategy s;
    // TC = 0 / (156/52) = 0: no deviation → basic says HIT
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SEVEN), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::THREE), only_hit_stand(), 0, 156)), Decision::HIT);
}

TEST(HiLoTest, BetsMinimumAtNegativeTrueCount) {
    HiLoStrategy s;
    BettingConfig config;
    EXPECT_EQ(s.get_bet_size(-2.0f, config), config.get_min_bet());
}

TEST(HiLoTest, BetsMinimumAtTrueCountOne) {
    HiLoStrategy s;
    BettingConfig config;
    EXPECT_EQ(s.get_bet_size(1.0f, config), config.get_min_bet());
}

TEST(HiLoTest, BetScalesWithTrueCount) {
    HiLoStrategy s;
    BettingConfig config;
    uint32_t bet_at_tc2 = s.get_bet_size(2.0f, config);
    uint32_t bet_at_tc4 = s.get_bet_size(4.0f, config);
    EXPECT_GT(bet_at_tc2, config.get_min_bet());
    EXPECT_GT(bet_at_tc4, bet_at_tc2);
}

TEST(HiLoTest, BetCappedAtMaxBet) {
    HiLoStrategy s;
    BettingConfig config;
    EXPECT_LE(s.get_bet_size(100.0f, config), config.get_max_bet());
}
