#include <blackjack/card/card.hpp>
#include <blackjack/card/rank.hpp>
#include <blackjack/card/suit.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/game/game.hpp>
#include <blackjack/game/ruleset.hpp>
#include <blackjack/hand/hand.hpp>
#include <blackjack/player/strategy/basic.hpp>
#include <blackjack/player/strategy/bearish.hpp>
#include <blackjack/player/strategy/bullish.hpp>
#include <blackjack/player/strategy/mimic_dealer.hpp>
#include <blackjack/player/strategy/strategy.hpp>

#include <gtest/gtest.h>

#include <memory>

using blackjack::card::Card;
using blackjack::card::Rank;
using blackjack::card::Suit;
using blackjack::game::BettingConfig;
using blackjack::game::Game;
using blackjack::game::Ruleset;
using blackjack::hand::Hand;
using blackjack::player::strategy::BasicStrategy;
using blackjack::player::strategy::BearishStrategy;
using blackjack::player::strategy::BullishStrategy;
using blackjack::player::strategy::Decision;
using blackjack::player::strategy::GameContext;
using blackjack::player::strategy::LegalActions;
using blackjack::player::strategy::MimicDealerStrategy;
using blackjack::player::strategy::PlayerStrategy;

namespace {
    Hand make_hand(Card a, Card b) {
        Hand h;
        h.add_card(a);
        h.add_card(b);
        return h;
    }

    GameContext ctx(const Hand& hand, Card upcard, LegalActions legal) {
        return GameContext(hand, upcard, legal);
    }

    constexpr LegalActions all_legal()          { return LegalActions{true,  true,  true};  }
    constexpr LegalActions only_hit_stand()     { return LegalActions{false, false, false}; }
    constexpr LegalActions can_double_only()    { return LegalActions{true,  false, false}; }
    constexpr LegalActions can_surrender_only() { return LegalActions{false, false, true};  }
}

TEST(MimicDealerStrategyTest, HitsBelow17) {
    MimicDealerStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), all_legal())), Decision::HIT);
}

TEST(MimicDealerStrategyTest, StandsAtHard17) {
    MimicDealerStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SEVEN));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::SIX), all_legal())), Decision::STAND);
}

TEST(MimicDealerStrategyTest, StandsOnSoft17WhenS17) {
    MimicDealerStrategy s(/*hits_soft_17=*/false);
    Hand hand = make_hand(Card(Suit::SPADES, Rank::ACE), Card(Suit::HEARTS, Rank::SIX));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::STAND);
}

TEST(MimicDealerStrategyTest, HitsOnSoft17WhenH17) {
    MimicDealerStrategy s(/*hits_soft_17=*/true);
    Hand hand = make_hand(Card(Suit::SPADES, Rank::ACE), Card(Suit::HEARTS, Rank::SIX));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::HIT);
}

TEST(MimicDealerStrategyTest, NeverDoublesSplitsOrSurrendersEvenWhenLegal) {
    MimicDealerStrategy s;
    Hand eleven = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    Hand pair_eights = make_hand(Card(Suit::SPADES, Rank::EIGHT), Card(Suit::HEARTS, Rank::EIGHT));
    Hand sixteen = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));

    EXPECT_EQ(s.get_decision(ctx(eleven, Card(Suit::CLUBS, Rank::SIX), all_legal())), Decision::HIT);
    EXPECT_NE(s.get_decision(ctx(pair_eights, Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::SPLIT);
    EXPECT_NE(s.get_decision(ctx(sixteen, Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::SURRENDER);
}

TEST(BasicStrategyTest, HitsHard8) {
    BasicStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::FIVE), Card(Suit::HEARTS, Rank::THREE));
    for (uint8_t up = 2; up <= 11; up++) {
        Rank rank = (up == 11) ? Rank::ACE : static_cast<Rank>(up);
        Card upcard(Suit::CLUBS, rank);
        EXPECT_EQ(s.get_decision(ctx(hand, upcard, all_legal())), Decision::HIT)
            << "hard 8 vs up=" << int(up);
    }
}

TEST(BasicStrategyTest, DoublesHard11WhenAllowed) {
    BasicStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), can_double_only())), Decision::DOUBLE);
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::ACE), can_double_only())), Decision::DOUBLE);
}

TEST(BasicStrategyTest, FallsBackToHitWhenDoubleIllegal) {
    BasicStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), only_hit_stand())), Decision::HIT);
}

TEST(BasicStrategyTest, DoublesHard10OnlyAgainstTwoThroughNine) {
    BasicStrategy s;
    Hand hand = make_hand(Card(Suit::SPADES, Rank::SEVEN), Card(Suit::HEARTS, Rank::THREE));
    for (uint8_t up = 2; up <= 9; up++) {
        Card upcard(Suit::CLUBS, static_cast<Rank>(up));
        EXPECT_EQ(s.get_decision(ctx(hand, upcard, can_double_only())), Decision::DOUBLE)
            << "hard 10 vs up=" << int(up);
    }
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::TEN), can_double_only())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(hand, Card(Suit::CLUBS, Rank::ACE), can_double_only())), Decision::HIT);
}

TEST(BasicStrategyTest, StandsOnStiffAgainstBustCards) {
    BasicStrategy s;
    Hand thirteen = make_hand(Card(Suit::SPADES, Rank::EIGHT), Card(Suit::HEARTS, Rank::FIVE));
    Hand sixteen  = make_hand(Card(Suit::SPADES, Rank::TEN),   Card(Suit::HEARTS, Rank::SIX));

    for (uint8_t up = 2; up <= 6; up++) {
        Card upcard(Suit::CLUBS, static_cast<Rank>(up));
        if (up >= 4) {
            EXPECT_EQ(s.get_decision(ctx(thirteen, upcard, all_legal())), Decision::STAND);
        }
        EXPECT_EQ(s.get_decision(ctx(sixteen, upcard, all_legal())), Decision::STAND);
    }
}

TEST(BasicStrategyTest, HitsTwelveAgainstDealerTwoAndThree) {
    BasicStrategy s;
    Hand twelve = make_hand(Card(Suit::SPADES, Rank::SEVEN), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(twelve, Card(Suit::CLUBS, Rank::TWO), all_legal())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(twelve, Card(Suit::CLUBS, Rank::THREE), all_legal())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(twelve, Card(Suit::CLUBS, Rank::FOUR), all_legal())), Decision::STAND);
}

TEST(BasicStrategyTest, SurrendersSixteenAgainstStrongUpcardsWhenAllowed) {
    BasicStrategy s;
    Hand sixteen = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));

    EXPECT_EQ(s.get_decision(ctx(sixteen, Card(Suit::CLUBS, Rank::NINE), can_surrender_only())), Decision::SURRENDER);
    EXPECT_EQ(s.get_decision(ctx(sixteen, Card(Suit::CLUBS, Rank::TEN),  can_surrender_only())), Decision::SURRENDER);
    EXPECT_EQ(s.get_decision(ctx(sixteen, Card(Suit::CLUBS, Rank::ACE),  can_surrender_only())), Decision::SURRENDER);
}

TEST(BasicStrategyTest, FallsBackToHitSixteenWhenSurrenderDisallowed) {
    BasicStrategy s;
    Hand sixteen = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SIX));
    EXPECT_EQ(s.get_decision(ctx(sixteen, Card(Suit::CLUBS, Rank::TEN), only_hit_stand())), Decision::HIT);
}

TEST(BasicStrategyTest, SurrendersFifteenOnlyAgainstTen) {
    BasicStrategy s;
    Hand fifteen = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::FIVE));
    EXPECT_EQ(s.get_decision(ctx(fifteen, Card(Suit::CLUBS, Rank::TEN),  can_surrender_only())), Decision::SURRENDER);
    EXPECT_EQ(s.get_decision(ctx(fifteen, Card(Suit::CLUBS, Rank::NINE), can_surrender_only())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(fifteen, Card(Suit::CLUBS, Rank::ACE),  can_surrender_only())), Decision::HIT);
}

TEST(BasicStrategyTest, AlwaysSplitsAcesAndEights) {
    BasicStrategy s;
    Hand aces   = make_hand(Card(Suit::SPADES, Rank::ACE),   Card(Suit::HEARTS, Rank::ACE));
    Hand eights = make_hand(Card(Suit::SPADES, Rank::EIGHT), Card(Suit::HEARTS, Rank::EIGHT));

    for (uint8_t up = 2; up <= 11; up++) {
        Rank rank = (up == 11) ? Rank::ACE : static_cast<Rank>(up);
        Card upcard(Suit::CLUBS, rank);
        EXPECT_EQ(s.get_decision(ctx(aces,   upcard, all_legal())), Decision::SPLIT);
        EXPECT_EQ(s.get_decision(ctx(eights, upcard, all_legal())), Decision::SPLIT);
    }
}

TEST(BasicStrategyTest, NeverSplitsTens) {
    BasicStrategy s;
    Hand tens = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::TEN));
    for (uint8_t up = 2; up <= 11; up++) {
        Rank rank = (up == 11) ? Rank::ACE : static_cast<Rank>(up);
        Card upcard(Suit::CLUBS, rank);
        EXPECT_EQ(s.get_decision(ctx(tens, upcard, all_legal())), Decision::STAND);
    }
}

TEST(BasicStrategyTest, NeverSplitsFivesTreatsThemAsHardTen) {
    BasicStrategy s;
    Hand fives = make_hand(Card(Suit::SPADES, Rank::FIVE), Card(Suit::HEARTS, Rank::FIVE));

    EXPECT_EQ(s.get_decision(ctx(fives, Card(Suit::CLUBS, Rank::SIX),  all_legal())), Decision::DOUBLE);
    EXPECT_EQ(s.get_decision(ctx(fives, Card(Suit::CLUBS, Rank::TEN),  all_legal())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(fives, Card(Suit::CLUBS, Rank::ACE),  all_legal())), Decision::HIT);
}

TEST(BasicStrategyTest, SplitsNinesExceptAgainstSevenTenAndAce) {
    BasicStrategy s;
    Hand nines = make_hand(Card(Suit::SPADES, Rank::NINE), Card(Suit::HEARTS, Rank::NINE));

    EXPECT_EQ(s.get_decision(ctx(nines, Card(Suit::CLUBS, Rank::SIX),   all_legal())), Decision::SPLIT);
    EXPECT_EQ(s.get_decision(ctx(nines, Card(Suit::CLUBS, Rank::SEVEN), all_legal())), Decision::STAND);
    EXPECT_EQ(s.get_decision(ctx(nines, Card(Suit::CLUBS, Rank::EIGHT), all_legal())), Decision::SPLIT);
    EXPECT_EQ(s.get_decision(ctx(nines, Card(Suit::CLUBS, Rank::TEN),   all_legal())), Decision::STAND);
    EXPECT_EQ(s.get_decision(ctx(nines, Card(Suit::CLUBS, Rank::ACE),   all_legal())), Decision::STAND);
}

TEST(BasicStrategyTest, SoftEighteenDoublesVsWeakStandsVsMediumHitsVsStrong) {
    BasicStrategy s;
    Hand soft18 = make_hand(Card(Suit::SPADES, Rank::ACE), Card(Suit::HEARTS, Rank::SEVEN));

    EXPECT_EQ(s.get_decision(ctx(soft18, Card(Suit::CLUBS, Rank::TWO),   can_double_only())), Decision::STAND);
    EXPECT_EQ(s.get_decision(ctx(soft18, Card(Suit::CLUBS, Rank::FOUR),  can_double_only())), Decision::DOUBLE);
    EXPECT_EQ(s.get_decision(ctx(soft18, Card(Suit::CLUBS, Rank::SEVEN), can_double_only())), Decision::STAND);
    EXPECT_EQ(s.get_decision(ctx(soft18, Card(Suit::CLUBS, Rank::NINE),  can_double_only())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(soft18, Card(Suit::CLUBS, Rank::ACE),   can_double_only())), Decision::HIT);
}

TEST(BasicStrategyTest, StandsOnSoft19AndSoft20) {
    BasicStrategy s;
    Hand soft19 = make_hand(Card(Suit::SPADES, Rank::ACE), Card(Suit::HEARTS, Rank::EIGHT));
    Hand soft20 = make_hand(Card(Suit::SPADES, Rank::ACE), Card(Suit::HEARTS, Rank::NINE));
    for (uint8_t up = 2; up <= 11; up++) {
        Rank rank = (up == 11) ? Rank::ACE : static_cast<Rank>(up);
        Card upcard(Suit::CLUBS, rank);
        EXPECT_EQ(s.get_decision(ctx(soft19, upcard, all_legal())), Decision::STAND);
        EXPECT_EQ(s.get_decision(ctx(soft20, upcard, all_legal())), Decision::STAND);
    }
}

TEST(BasicStrategyTest, OnlySplitsFoursAgainstFiveAndSixWhenDasAllowed) {
    BasicStrategy das_on(true);
    BasicStrategy das_off(false);
    Hand fours = make_hand(Card(Suit::SPADES, Rank::FOUR), Card(Suit::HEARTS, Rank::FOUR));

    EXPECT_EQ(das_on.get_decision(ctx(fours, Card(Suit::CLUBS, Rank::FIVE), all_legal())), Decision::SPLIT);
    EXPECT_EQ(das_on.get_decision(ctx(fours, Card(Suit::CLUBS, Rank::SIX),  all_legal())), Decision::SPLIT);
    EXPECT_EQ(das_on.get_decision(ctx(fours, Card(Suit::CLUBS, Rank::FOUR), all_legal())), Decision::HIT);

    EXPECT_EQ(das_off.get_decision(ctx(fours, Card(Suit::CLUBS, Rank::FIVE), all_legal())), Decision::HIT);
    EXPECT_EQ(das_off.get_decision(ctx(fours, Card(Suit::CLUBS, Rank::SIX),  all_legal())), Decision::HIT);
}

TEST(BearishStrategyTest, HitsBelowTwelveAndStandsOtherwise) {
    BearishStrategy s;
    Hand eleven = make_hand(Card(Suit::SPADES, Rank::SIX), Card(Suit::HEARTS, Rank::FIVE));
    Hand twelve = make_hand(Card(Suit::SPADES, Rank::SEVEN), Card(Suit::HEARTS, Rank::FIVE));
    Hand seventeen = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::SEVEN));

    EXPECT_EQ(s.get_decision(ctx(eleven,    Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(twelve,    Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::STAND);
    EXPECT_EQ(s.get_decision(ctx(seventeen, Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::STAND);
}

TEST(BullishStrategyTest, HitsUntilTwentyOne) {
    BullishStrategy s;
    Hand twenty = make_hand(Card(Suit::SPADES, Rank::TEN), Card(Suit::HEARTS, Rank::TEN));
    Hand five   = make_hand(Card(Suit::SPADES, Rank::THREE), Card(Suit::HEARTS, Rank::TWO));

    EXPECT_EQ(s.get_decision(ctx(twenty, Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::HIT);
    EXPECT_EQ(s.get_decision(ctx(five,   Card(Suit::CLUBS, Rank::TEN), all_legal())), Decision::HIT);
}

namespace {
    int64_t delta_with_strategy(auto factory, uint64_t seed) {
        Game game(BettingConfig{}, Ruleset::default_vegas());
        game.initialize_round();
        for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
            game.get_player(i).set_strategy(factory());
        }
        game.play_round(seed);
        return game.aggregate_statistics().get_bankroll_delta();
    }
}

TEST(StrategySelectionTest, DifferentStrategiesProduceDifferentOutcomesOnSameSeed) {
    bool found_difference = false;
    for (uint64_t seed = 1; seed <= 30 && !found_difference; seed++) {
        int64_t basic   = delta_with_strategy([]{ return std::make_unique<BasicStrategy>();        }, seed);
        int64_t mimic   = delta_with_strategy([]{ return std::make_unique<MimicDealerStrategy>();  }, seed);
        int64_t bullish = delta_with_strategy([]{ return std::make_unique<BullishStrategy>();      }, seed);

        if (basic != mimic || basic != bullish || mimic != bullish) {
            found_difference = true;
        }
    }
    EXPECT_TRUE(found_difference);
}

TEST(StrategySelectionTest, SameStrategyOnSameSeedIsDeterministic) {
    int64_t a = delta_with_strategy([]{ return std::make_unique<BasicStrategy>(); }, 7);
    int64_t b = delta_with_strategy([]{ return std::make_unique<BasicStrategy>(); }, 7);
    EXPECT_EQ(a, b);
}

TEST(StrategySelectionTest, PlayerStrategyIsWhatTheCallerSets) {
    Game game(BettingConfig{}, Ruleset::default_vegas());
    game.initialize_round();

    for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
        game.get_player(i).set_strategy(std::make_unique<BearishStrategy>());
    }
    for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
        PlayerStrategy* assigned = game.get_player(i).get_strategy();
        ASSERT_NE(assigned, nullptr);
        EXPECT_NE(dynamic_cast<BearishStrategy*>(assigned), nullptr);
    }
}
