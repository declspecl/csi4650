#include <blackjack/game/game.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/hand/hand_origin.hpp>
#include <blackjack/hand/hand_outcome.hpp>
#include <blackjack/player/strategy/bearish.hpp>
#include <blackjack/player/strategy/bullish.hpp>
#include <blackjack/player/strategy/mimic_dealer.hpp>
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
using blackjack::hand::HandOrigin;
using blackjack::hand::HandOutcome;
using blackjack::player::strategy::BearishStrategy;
using blackjack::player::strategy::Decision;
using blackjack::player::strategy::MimicDealerStrategy;

template <typename StrategyT>
static void seat_all_players(Game& game) {
    for (uint8_t i = 0; i < Game::MAX_NON_DEALER_PLAYERS; i++) {
        game.get_player(i).set_strategy(std::make_unique<StrategyT>());
    }
}

static Game make_game() {
    BettingConfig config;
    Game game(config);
    game.initialize_round();
    seat_all_players<MimicDealerStrategy>(game);
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

TEST(RoundTest, DoubleDrawsOneCardAndDoublesBet) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::FIVE),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    ActionApplicationResult result = game.apply_decision(0, 0, Decision::DOUBLE);

    EXPECT_EQ(result, ActionApplicationResult::APPLIED_TURN_COMPLETE);
    EXPECT_EQ(game.get_player(0).get_hand(0).card_count(), 3);
    EXPECT_EQ(game.get_player(0).get_hand(0).get_bet(), 200u);
}

TEST(RoundTest, SplitCreatesSecondActiveHand) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );

    ActionApplicationResult result = game.apply_decision(0, 0, Decision::SPLIT);

    EXPECT_EQ(result, ActionApplicationResult::APPLIED_CONTINUE);
    EXPECT_EQ(game.get_player(0).get_active_hand_count(), 2);
    EXPECT_EQ(game.get_player(0).get_hand(0).card_count(), 2);
    EXPECT_EQ(game.get_player(0).get_hand(1).card_count(), 2);
}

TEST(RoundTest, SurrenderReturnHalfBetAndEndsTurn) {
    Game game = make_game();
    set_player_hand(
        game,
        0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t bankroll_before = game.get_player(0).get_bankroll();

    ActionApplicationResult result = game.apply_decision(0, 0, Decision::SURRENDER);

    EXPECT_EQ(result, ActionApplicationResult::APPLIED_TURN_COMPLETE);
    EXPECT_EQ(game.get_player(0).get_bankroll(), bankroll_before + 50);
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

    EXPECT_EQ(game.apply_decision(0, 1, Decision::HIT), ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 1, Decision::STAND), ActionApplicationResult::ILLEGAL_ACTION);
}

TEST(RoundTest, HitUntilBustCompletesTurnAndRejectsFurtherActions) {
    Game game = make_game();
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

    EXPECT_EQ(result, ActionApplicationResult::APPLIED_TURN_COMPLETE);
    EXPECT_TRUE(game.get_player(0).get_hand(0).is_bust());

    EXPECT_EQ(game.apply_decision(0, 0, Decision::HIT), ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::STAND), ActionApplicationResult::ILLEGAL_ACTION);
}

static void set_dealer_hand(Game& game, Card first, Card second) {
    auto& dealer = game.get_dealer();
    dealer.clear_hand(0);
    dealer.add_card_to_hand(0, first);
    dealer.add_card_to_hand(0, second);
}

static void setup_hand_with_placed_bet(
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
    player.deduct_from_bankroll(bet);
}

static void replace_last_card(Hand& hand, Card replacement) {
    (void) hand.pop_card();
    hand.add_card(replacement);
}

TEST(ActionLifecycleTest, DoubleBlocksAllSubsequentActions) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::FIVE),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    ASSERT_EQ(game.apply_decision(0, 0, Decision::DOUBLE), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    EXPECT_EQ(game.apply_decision(0, 0, Decision::HIT),       ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::STAND),     ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::DOUBLE),    ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::SPLIT),     ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::ILLEGAL_ACTION);
}

TEST(ActionLifecycleTest, DoubleWinPaysTwoToOneOnDoubledStake) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::FIVE),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::DOUBLE), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::NINE));
    ASSERT_EQ(game.get_player(0).get_hand(0).get_value(), 20u);
    ASSERT_EQ(game.get_player(0).get_hand(0).get_bet(), 200u);

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::NINE), Card(Suit::HEARTS, Rank::NINE));

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll + 200);
}

TEST(ActionLifecycleTest, DoubleLossLosesDoubledStake) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::FIVE),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::DOUBLE), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::FOUR));
    ASSERT_EQ(game.get_player(0).get_hand(0).get_value(), 15u);

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::NINE), Card(Suit::HEARTS, Rank::NINE));

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll - 200);
}

TEST(ActionLifecycleTest, DoubleBustLosesDoubledStake) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::DOUBLE), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::KING));
    ASSERT_TRUE(game.get_player(0).get_hand(0).is_bust());

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::NINE), Card(Suit::HEARTS, Rank::EIGHT));

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll - 200);
}

TEST(ActionLifecycleTest, DoublePushReturnsDoubledStake) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::FIVE),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::DOUBLE), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::SEVEN));
    ASSERT_EQ(game.get_player(0).get_hand(0).get_value(), 18u);

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::NINE), Card(Suit::HEARTS, Rank::NINE));

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll);
}

TEST(ActionLifecycleTest, SurrenderBlocksAllSubsequentActions) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    EXPECT_EQ(game.apply_decision(0, 0, Decision::HIT),       ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::STAND),     ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::DOUBLE),    ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::SPLIT),     ActionApplicationResult::ILLEGAL_ACTION);
    EXPECT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::ILLEGAL_ACTION);
}

TEST(ActionLifecycleTest, SurrenderOutcomeReportsSurrenderLossWithZeroPayout) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    ASSERT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    const Hand& player_hand = game.get_player(0).get_hand(0);
    Hand dealer_hand;
    dealer_hand.add_card(Card(Suit::DIAMONDS, Rank::NINE));
    dealer_hand.add_card(Card(Suit::CLUBS, Rank::NINE));

    EXPECT_EQ(game.determine_outcome(player_hand, dealer_hand), HandOutcome::SURRENDER_LOSS);
    EXPECT_EQ(game.calculate_payout(HandOutcome::SURRENDER_LOSS, 100), 0u);
}

TEST(ActionLifecycleTest, SurrenderSettlementSkipsRegularPayoutOnDealerBust) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::KING), Card(Suit::CLUBS, Rank::KING));
    game.get_dealer().add_card_to_hand(0, Card(Suit::HEARTS, Rank::FIVE));
    ASSERT_TRUE(game.get_dealer().get_hand(0).is_bust());

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll - 50);
    EXPECT_EQ(game.aggregate_statistics().get_hands_played(), 1u);
}

TEST(ActionLifecycleTest, SurrenderSettlementSkipsRegularPayoutOnDealerPush) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::NINE), Card(Suit::CLUBS, Rank::SEVEN));

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll - 50);
}

TEST(ActionLifecycleTest, SurrenderSettlementSkipsRegularPayoutOnPlayerHigherValue) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::TEN), 
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::NINE), Card(Suit::CLUBS, Rank::NINE));

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll - 50);
}

TEST(ActionLifecycleTest, SurrenderAgainstDealerBlackjackStillOnlyLosesHalf) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::TEN),
        Card(Suit::HEARTS, Rank::SIX),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SURRENDER), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    set_dealer_hand(game, Card(Suit::DIAMONDS, Rank::ACE), Card(Suit::CLUBS, Rank::KING));
    ASSERT_TRUE(game.get_dealer().get_hand(0).is_blackjack());

    game.resolve_hand(game.get_player(0), 0);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll - 50);
}

TEST(ActionLifecycleTest, SplitDeductsSecondBetAndMarksBothHandsSplitOrigin) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    uint32_t bankroll_before_split = game.get_player(0).get_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);

    auto& player = game.get_player(0);
    EXPECT_EQ(player.get_bankroll(), bankroll_before_split - 100);
    EXPECT_EQ(player.get_active_hand_count(), 2);
    EXPECT_EQ(player.get_hand(0).get_bet(), 100u);
    EXPECT_EQ(player.get_hand(1).get_bet(), 100u);
    EXPECT_EQ(player.get_hand(0).get_origin(), HandOrigin::SPLIT);
    EXPECT_EQ(player.get_hand(1).get_origin(), HandOrigin::SPLIT);
}

TEST(ActionLifecycleTest, SplitBothHandsWinSettledIndependently) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);

    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::TEN));
    replace_last_card(game.get_player(0).get_hand(1), Card(Suit::DIAMONDS, Rank::TEN));

    ASSERT_EQ(game.apply_decision(0, 0, Decision::STAND), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    ASSERT_EQ(game.apply_decision(0, 1, Decision::STAND), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    set_dealer_hand(game, Card(Suit::HEARTS, Rank::TEN), Card(Suit::SPADES, Rank::SEVEN));

    game.resolve_hand(game.get_player(0), 0);
    game.resolve_hand(game.get_player(0), 1);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll + 200);
    EXPECT_EQ(game.aggregate_statistics().get_hands_played(), 2u);
}

TEST(ActionLifecycleTest, SplitOneWinOneLossSettlesEachHandSeparately) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);

    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::TEN));
    replace_last_card(game.get_player(0).get_hand(1), Card(Suit::DIAMONDS, Rank::FIVE));

    ASSERT_EQ(game.apply_decision(0, 0, Decision::STAND), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    ASSERT_EQ(game.apply_decision(0, 1, Decision::STAND), ActionApplicationResult::APPLIED_TURN_COMPLETE);

    set_dealer_hand(game, Card(Suit::HEARTS, Rank::TEN), Card(Suit::SPADES, Rank::SEVEN));

    game.resolve_hand(game.get_player(0), 0);
    game.resolve_hand(game.get_player(0), 1);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll);
    EXPECT_EQ(game.aggregate_statistics().get_hands_played(), 2u);
}

TEST(ActionLifecycleTest, SplitTwentyOneDoesNotPayBlackjackOdds) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::ACE),
        Card(Suit::HEARTS, Rank::ACE),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);
    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::KING));
    replace_last_card(game.get_player(0).get_hand(1), Card(Suit::DIAMONDS, Rank::QUEEN));

    ASSERT_EQ(game.get_player(0).get_hand(0).get_value(), 21u);
    ASSERT_EQ(game.get_player(0).get_hand(1).get_value(), 21u);

    set_dealer_hand(game, Card(Suit::HEARTS, Rank::TEN), Card(Suit::SPADES, Rank::SEVEN));

    game.resolve_hand(game.get_player(0), 0);
    game.resolve_hand(game.get_player(0), 1);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll + 200);
}

TEST(ActionLifecycleTest, SplitHandBustOnlyLosesThatHandsStake) {
    Game game = make_game();
    setup_hand_with_placed_bet(
        game, 0,
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::EIGHT),
        100
    );
    uint32_t initial_bankroll = game.get_betting_config().get_initial_bankroll();

    ASSERT_EQ(game.apply_decision(0, 0, Decision::SPLIT), ActionApplicationResult::APPLIED_CONTINUE);

    replace_last_card(game.get_player(0).get_hand(0), Card(Suit::CLUBS, Rank::TEN));
    replace_last_card(game.get_player(0).get_hand(1), Card(Suit::DIAMONDS, Rank::FIVE));

    ASSERT_EQ(game.apply_decision(0, 0, Decision::STAND), ActionApplicationResult::APPLIED_TURN_COMPLETE);
    game.get_player(0).get_hand(1).add_card(Card(Suit::CLUBS, Rank::KING));
    ASSERT_TRUE(game.get_player(0).get_hand(1).is_bust());

    set_dealer_hand(game, Card(Suit::HEARTS, Rank::TEN), Card(Suit::SPADES, Rank::SEVEN));

    game.resolve_hand(game.get_player(0), 0);
    game.resolve_hand(game.get_player(0), 1);

    EXPECT_EQ(game.get_player(0).get_bankroll(), initial_bankroll);
}
