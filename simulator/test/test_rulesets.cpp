#include <blackjack/card/card.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/game/game.hpp>
#include <blackjack/game/ruleset.hpp>
#include <blackjack/hand/hand.hpp>
#include <blackjack/hand/hand_origin.hpp>
#include <blackjack/hand/hand_outcome.hpp>
#include <blackjack/player/strategy/dealer.hpp>
#include <blackjack/player/strategy/strategy.hpp>

#include <gtest/gtest.h>

using blackjack::card::Card;
using blackjack::card::Rank;
using blackjack::card::Suit;
using blackjack::game::ActionApplicationResult;
using blackjack::game::BettingConfig;
using blackjack::game::Game;
using blackjack::game::Ruleset;
using blackjack::hand::Hand;
using blackjack::hand::HandOrigin;
using blackjack::hand::HandOutcome;
using blackjack::player::strategy::DealerStrategy;
using blackjack::player::strategy::Decision;
using blackjack::player::strategy::GameContext;
using blackjack::player::strategy::LegalActions;

namespace {
    Game make_game_with_rules(const Ruleset& rules) {
        Game game(BettingConfig{}, rules);
        game.initialize_round();
        return game;
    }

    void seat_hand(
        Game& game,
        uint8_t player_index,
        Card first,
        Card second,
        uint32_t bet,
        HandOrigin origin = HandOrigin::NATURAL
    ) {
        auto& player = game.get_player(player_index);
        player.clear_hand(0);
        player.set_active_hand_count(1);
        player.add_card_to_hand(0, first);
        player.add_card_to_hand(0, second);
        player.set_hand_bet(0, bet);
        player.get_hand(0).set_origin(origin);
        player.deduct_from_bankroll(bet);
    }
}

TEST(RulesetTest, DefaultVegasFactoryMatchesHistoricalBehavior) {
    Ruleset defaults = Ruleset::default_vegas();
    EXPECT_EQ(defaults.blackjack_payout_num, 3);
    EXPECT_EQ(defaults.blackjack_payout_denom, 2);
    EXPECT_FALSE(defaults.dealer_hits_soft_17);
    EXPECT_TRUE(defaults.surrender_allowed);
    EXPECT_TRUE(defaults.double_after_split_allowed);
    EXPECT_EQ(defaults.max_split_hands, 4);
}

TEST(RulesetTest, NamedFactoriesAreDistinguishable) {
    Ruleset vegas = Ruleset::default_vegas();
    Ruleset tight = Ruleset::tight_h17_no_surrender();

    EXPECT_NE(vegas.blackjack_payout_num,        tight.blackjack_payout_num);
    EXPECT_NE(vegas.dealer_hits_soft_17,         tight.dealer_hits_soft_17);
    EXPECT_NE(vegas.surrender_allowed,           tight.surrender_allowed);
    EXPECT_NE(vegas.double_after_split_allowed,  tight.double_after_split_allowed);
    EXPECT_NE(vegas.max_split_hands,             tight.max_split_hands);
}

TEST(RulesetPayoutTest, BlackjackPays3to2UnderDefault) {
    Game game = make_game_with_rules(Ruleset::default_vegas());
    EXPECT_EQ(game.calculate_payout(HandOutcome::BLACKJACK_WIN, 1000), 2500u);
}

TEST(RulesetPayoutTest, BlackjackPays6to5UnderTightRuleset) {
    Game game = make_game_with_rules(Ruleset::tight_h17_no_surrender());
    EXPECT_EQ(game.calculate_payout(HandOutcome::BLACKJACK_WIN, 1000), 2200u);
}

TEST(RulesetPayoutTest, SameOutcomeYieldsDifferentPayoutsAcrossRulesets) {
    Game vegas_game = make_game_with_rules(Ruleset::default_vegas());
    Game tight_game = make_game_with_rules(Ruleset::tight_h17_no_surrender());

    uint32_t vegas_payout = vegas_game.calculate_payout(HandOutcome::BLACKJACK_WIN, 1000);
    uint32_t tight_payout = tight_game.calculate_payout(HandOutcome::BLACKJACK_WIN, 1000);

    EXPECT_GT(vegas_payout, tight_payout);
}

TEST(RulesetLegalActionsTest, SurrenderLegalUnderDefault) {
    Game game = make_game_with_rules(Ruleset::default_vegas());
    seat_hand(game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    LegalActions legal = game.get_legal_actions(0, 0);
    EXPECT_TRUE(legal.can_surrender);
}

TEST(RulesetLegalActionsTest, SurrenderIllegalWhenDisallowedByRuleset) {
    Ruleset no_surrender = Ruleset::default_vegas();
    no_surrender.surrender_allowed = false;

    Game game = make_game_with_rules(no_surrender);
    seat_hand(game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    LegalActions legal = game.get_legal_actions(0, 0);
    EXPECT_FALSE(legal.can_surrender);
    EXPECT_EQ(
        game.apply_decision(0, 0, Decision::SURRENDER),
        ActionApplicationResult::ILLEGAL_ACTION
    );
}

TEST(RulesetLegalActionsTest, DoubleAfterSplitLegalUnderDefault) {
    Game game = make_game_with_rules(Ruleset::default_vegas());
    seat_hand(game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);

    auto& post_split = game.get_player(0).get_hand(1);
    (void) post_split.pop_card();
    post_split.add_card(Card(Suit::CLUBS, Rank::THREE));

    LegalActions legal = game.get_legal_actions(0, 1);
    EXPECT_TRUE(legal.can_double)
        << "Default ruleset must permit doubling on a post-split hand";
}

TEST(RulesetLegalActionsTest, DoubleAfterSplitIllegalWhenDisallowed) {
    Ruleset no_das = Ruleset::default_vegas();
    no_das.double_after_split_allowed = false;

    Game game = make_game_with_rules(no_das);
    seat_hand(game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);

    auto& post_split = game.get_player(0).get_hand(1);
    (void) post_split.pop_card();
    post_split.add_card(Card(Suit::CLUBS, Rank::THREE));

    LegalActions legal = game.get_legal_actions(0, 1);
    EXPECT_FALSE(legal.can_double);
    EXPECT_EQ(
        game.apply_decision(0, 1, Decision::DOUBLE),
        ActionApplicationResult::ILLEGAL_ACTION
    );
}

TEST(RulesetLegalActionsTest, MaxSplitHandsCapsFurtherSplits) {
    Ruleset two_split_max = Ruleset::default_vegas();
    two_split_max.max_split_hands = 2;

    Game game = make_game_with_rules(two_split_max);
    seat_hand(game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);
    ASSERT_EQ(game.get_player(0).get_active_hand_count(), 2);

    auto& hand0 = game.get_player(0).get_hand(0);
    (void) hand0.pop_card();
    hand0.add_card(Card(Suit::DIAMONDS, Rank::EIGHT));

    LegalActions legal = game.get_legal_actions(0, 0);
    EXPECT_FALSE(legal.can_split)
        << "Resplitting beyond max_split_hands must be disallowed";
    EXPECT_EQ(
        game.apply_decision(0, 0, Decision::SPLIT),
        ActionApplicationResult::ILLEGAL_ACTION
    );
}

TEST(RulesetLegalActionsTest, MaxSplitHandsFourAllowsResplit) {
    Ruleset four_split_max = Ruleset::default_vegas();
    four_split_max.max_split_hands = 4;

    Game game = make_game_with_rules(four_split_max);
    seat_hand(game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);

    auto& hand0 = game.get_player(0).get_hand(0);
    (void) hand0.pop_card();
    hand0.add_card(Card(Suit::DIAMONDS, Rank::EIGHT));

    LegalActions legal = game.get_legal_actions(0, 0);
    EXPECT_TRUE(legal.can_split);
}

namespace {
    Decision dealer_decision_for_soft_seventeen(bool hits_soft_17) {
        DealerStrategy strategy(hits_soft_17);
        Hand soft17;
        soft17.add_card(Card(Suit::HEARTS, Rank::ACE));
        soft17.add_card(Card(Suit::CLUBS, Rank::SIX));
        Card upcard(Suit::SPADES, Rank::TEN);
        GameContext ctx(soft17, upcard, LegalActions::none());
        return strategy.get_decision(ctx);
    }

    Decision dealer_decision_for_hard_seventeen(bool hits_soft_17) {
        DealerStrategy strategy(hits_soft_17);
        Hand hard17;
        hard17.add_card(Card(Suit::HEARTS, Rank::TEN));
        hard17.add_card(Card(Suit::CLUBS, Rank::SEVEN));
        Card upcard(Suit::SPADES, Rank::TEN);
        GameContext ctx(hard17, upcard, LegalActions::none());
        return strategy.get_decision(ctx);
    }
}

TEST(RulesetDealerTest, DealerStandsOnSoft17UnderS17) {
    EXPECT_EQ(dealer_decision_for_soft_seventeen(/*hits_soft_17=*/false), Decision::STAND);
}

TEST(RulesetDealerTest, DealerHitsSoft17UnderH17) {
    EXPECT_EQ(dealer_decision_for_soft_seventeen(/*hits_soft_17=*/true), Decision::HIT);
}

TEST(RulesetDealerTest, DealerAlwaysStandsOnHard17) {
    EXPECT_EQ(dealer_decision_for_hard_seventeen(/*hits_soft_17=*/false), Decision::STAND);
    EXPECT_EQ(dealer_decision_for_hard_seventeen(/*hits_soft_17=*/true),  Decision::STAND);
}

TEST(RulesetDealerTest, DealerHitsBelowSeventeenRegardless) {
    for (bool h17 : {false, true}) {
        DealerStrategy strategy(h17);
        Hand h;
        h.add_card(Card(Suit::HEARTS, Rank::TEN));
        h.add_card(Card(Suit::CLUBS, Rank::SIX));
        Card upcard(Suit::SPADES, Rank::TEN);
        GameContext ctx(h, upcard, LegalActions::none());
        EXPECT_EQ(strategy.get_decision(ctx), Decision::HIT);
    }
}

TEST(RulesetDealerTest, GameUsesDealerStrategyMatchingRuleset) {
    Game s17_game(BettingConfig{}, Ruleset::default_vegas());
    Game h17_game(BettingConfig{}, Ruleset::tight_h17_no_surrender());
    EXPECT_FALSE(s17_game.get_ruleset().dealer_hits_soft_17);
    EXPECT_TRUE(h17_game.get_ruleset().dealer_hits_soft_17);
}

TEST(RulesetEndToEndTest, SameSeedProducesDifferentDeltasAcrossRulesets) {
    Game vegas(BettingConfig{}, Ruleset::default_vegas());
    vegas.initialize_round();

    Game tight(BettingConfig{}, Ruleset::tight_h17_no_surrender());
    tight.initialize_round();

    bool found_difference = false;
    for (uint64_t seed = 1; seed <= 30 && !found_difference; seed++) {
        Game v(BettingConfig{}, Ruleset::default_vegas());
        v.initialize_round();
        v.play_round(seed);

        Game t(BettingConfig{}, Ruleset::tight_h17_no_surrender());
        t.initialize_round();
        t.play_round(seed);

        if (v.aggregate_statistics().get_bankroll_delta()
            != t.aggregate_statistics().get_bankroll_delta()) {
            found_difference = true;
        }
    }

    EXPECT_TRUE(found_difference)
        << "At least one seed must produce distinguishable outcomes between rulesets";
}

TEST(HandSoftnessTest, SoftHandDetection) {
    Hand h;
    h.add_card(Card(Suit::SPADES, Rank::ACE));
    h.add_card(Card(Suit::HEARTS, Rank::SIX));
    EXPECT_TRUE(h.is_soft());
    EXPECT_EQ(h.get_value(), 17u);
}

TEST(HandSoftnessTest, HardHandDetection) {
    Hand h;
    h.add_card(Card(Suit::SPADES, Rank::TEN));
    h.add_card(Card(Suit::HEARTS, Rank::SEVEN));
    EXPECT_FALSE(h.is_soft());
    EXPECT_EQ(h.get_value(), 17u);
}

TEST(HandSoftnessTest, AceDemotedToOneIsHard) {
    Hand h;
    h.add_card(Card(Suit::SPADES, Rank::ACE));
    h.add_card(Card(Suit::HEARTS, Rank::EIGHT));
    h.add_card(Card(Suit::CLUBS, Rank::SEVEN));
    EXPECT_FALSE(h.is_soft());
    EXPECT_EQ(h.get_value(), 16u);
}
