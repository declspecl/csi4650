#ifndef BLACKJACK_GAME_HPP
#define BLACKJACK_GAME_HPP

#include <blackjack/player/player.hpp>
#include <blackjack/deck/shoe.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/game/game_statistics.hpp>
#include <blackjack/game/hand_round_state.hpp>
#include <blackjack/hand/hand_origin.hpp>
#include <blackjack/hand/hand_outcome.hpp>
#include <blackjack/player/strategy/dealer.hpp>

#include <array>
#include <cstdint>
#include <utility>

using blackjack::player::Player;
using blackjack::deck::Shoe;
using blackjack::hand::Hand;
using blackjack::hand::HandOutcome;
using blackjack::hand::HandOrigin;
using blackjack::player::strategy::Decision;
using blackjack::player::strategy::DealerStrategy;
using blackjack::player::strategy::GameContext;
using blackjack::player::strategy::LegalActions;
using blackjack::player::strategy::PlayerStrategy;

namespace blackjack::game {
    enum class ActionApplicationResult : uint8_t {
        APPLIED_CONTINUE,
        APPLIED_TURN_COMPLETE,
        ILLEGAL_ACTION,
        UNSUPPORTED_ACTION,
    };

    class Game {
    public:
        static constexpr uint8_t MAX_NON_DEALER_PLAYERS = 7;

    private:
        std::array<Player, MAX_NON_DEALER_PLAYERS> players;
        std::array<
            std::array<HandRoundState, Player::MAX_POSSIBLE_HANDS>,
            MAX_NON_DEALER_PLAYERS
        > player_hand_states;
        Shoe shoe;
        Player dealer;
        HandRoundState dealer_hand_state;
        uint8_t player_count;
        BettingConfig betting_config;
        uint64_t hands_played_count;
        uint32_t starting_bankroll_total;

    public:
        inline Game() noexcept;
        explicit inline Game(const BettingConfig& config) noexcept;

        inline void initialize_round() noexcept;
        inline void play_round(uint64_t seed) noexcept;

        [[nodiscard]] inline ActionApplicationResult apply_decision(
            uint8_t player_index,
            uint8_t hand_index,
            Decision decision
        ) noexcept;

        [[nodiscard]] inline HandOutcome determine_outcome(
            const Hand& player_hand,
            const Hand& dealer_hand
        ) const noexcept;

        inline void resolve_hand(Player& player, uint8_t hand_index) noexcept;
        inline void resolve_all_hands() noexcept;

        [[nodiscard]] inline uint32_t calculate_payout(
            HandOutcome outcome,
            uint32_t bet
        ) const noexcept;

        [[nodiscard]] inline const BettingConfig& get_betting_config() const noexcept;
        [[nodiscard]] inline Player& get_player(uint8_t index) noexcept;
        [[nodiscard]] inline const Player& get_dealer() const noexcept;
        [[nodiscard]] inline Player& get_dealer() noexcept;
        [[nodiscard]] inline GameStatistics aggregate_statistics() const noexcept;

    private:
        [[nodiscard]] inline ActionApplicationResult apply_dealer_decision(Decision decision) noexcept;
        inline void play_turn_for_player(uint8_t player_index, uint8_t hand_index) noexcept;
        inline void play_dealer_turn() noexcept;
        [[nodiscard]] inline HandRoundState& get_player_hand_state(uint8_t player_index, uint8_t hand_index) noexcept;
        [[nodiscard]] inline const HandRoundState& get_player_hand_state(uint8_t player_index, uint8_t hand_index) const noexcept;
        [[nodiscard]] inline bool is_first_action(uint8_t player_index, uint8_t hand_index) const noexcept;
        [[nodiscard]] inline bool can_double(uint8_t player_index, uint8_t hand_index) const noexcept;
        [[nodiscard]] inline bool can_split(uint8_t player_index, uint8_t hand_index) const noexcept;
        [[nodiscard]] inline bool can_surrender(uint8_t player_index, uint8_t hand_index) const noexcept;
        [[nodiscard]] inline bool is_hand_turn_complete(uint8_t player_index, uint8_t hand_index) const noexcept;
        [[nodiscard]] inline bool is_dealer_turn_complete() const noexcept;
        [[nodiscard]] inline LegalActions make_legal_actions(
            uint8_t player_index,
            uint8_t hand_index
        ) const noexcept;

    };
}


namespace blackjack::game {
    Game::Game() noexcept
        : players({})
        , player_hand_states({})
        , shoe()
        , dealer()
        , dealer_hand_state()
        , player_count(MAX_NON_DEALER_PLAYERS)
        , betting_config()
        , hands_played_count(0)
        , starting_bankroll_total(0)
    {}

    Game::Game(const BettingConfig& config) noexcept
        : players({})
        , player_hand_states({})
        , shoe()
        , dealer()
        , dealer_hand_state()
        , player_count(MAX_NON_DEALER_PLAYERS)
        , betting_config(config)
        , hands_played_count(0)
        , starting_bankroll_total(0)
    {}

    void Game::initialize_round() noexcept {
        this->hands_played_count = 0;
        this->starting_bankroll_total = 0;

        for (uint8_t i = 0; i < this->player_count; i++) {
            this->players[i].initialize_bankroll(this->betting_config.get_initial_bankroll());
            this->starting_bankroll_total += this->betting_config.get_initial_bankroll();
        }

        this->dealer.clear_hand(0);
    }

    [[nodiscard]] inline ActionApplicationResult Game::apply_decision(
        uint8_t player_index,
        uint8_t hand_index,
        Decision decision
    ) noexcept {
        Player& player = this->players[player_index];
        if (hand_index >= player.get_active_hand_count()) {
            return ActionApplicationResult::ILLEGAL_ACTION;
        }

        if (this->is_hand_turn_complete(player_index, hand_index)) {
            return ActionApplicationResult::ILLEGAL_ACTION;
        }

        HandRoundState& state = this->get_player_hand_state(player_index, hand_index);

        switch (decision) {
            case Decision::HIT:
                player.add_card_to_hand(hand_index, this->shoe.draw());
                state.action_count++;
                return this->is_hand_turn_complete(player_index, hand_index)
                    ? ActionApplicationResult::APPLIED_TURN_COMPLETE
                    : ActionApplicationResult::APPLIED_CONTINUE;

            case Decision::STAND:
                state.stood = true;
                state.action_count++;
                return ActionApplicationResult::APPLIED_TURN_COMPLETE;

            case Decision::DOUBLE: {
                if (!this->can_double(player_index, hand_index)) {
                    return ActionApplicationResult::ILLEGAL_ACTION;
                }
                uint32_t original_bet = player.get_hand(hand_index).get_bet();
                player.deduct_from_bankroll(original_bet);
                player.set_hand_bet(hand_index, original_bet * 2);
                player.add_card_to_hand(hand_index, this->shoe.draw());
                state.doubled = true;
                state.action_count++;
                return ActionApplicationResult::APPLIED_TURN_COMPLETE;
            }

            case Decision::SPLIT: {
                if (!this->can_split(player_index, hand_index)) {
                    return ActionApplicationResult::ILLEGAL_ACTION;
                }
                uint8_t new_hand_index = player.get_active_hand_count();
                player.set_active_hand_count(new_hand_index + 1);

                Card split_card = player.get_hand(hand_index).pop_card();
                uint32_t split_bet = player.get_hand(hand_index).get_bet();

                player.clear_hand(new_hand_index);
                player.get_hand(new_hand_index).set_origin(HandOrigin::SPLIT);
                player.get_hand(new_hand_index).set_bet(split_bet);
                player.add_card_to_hand(new_hand_index, split_card);
                player.deduct_from_bankroll(split_bet);

                player.get_hand(hand_index).set_origin(HandOrigin::SPLIT);
                player.add_card_to_hand(hand_index, this->shoe.draw());
                player.add_card_to_hand(new_hand_index, this->shoe.draw());

                this->player_hand_states[player_index][new_hand_index].reset();
                state.action_count++;
                return ActionApplicationResult::APPLIED_CONTINUE;
            }

            case Decision::SURRENDER: {
                if (!this->can_surrender(player_index, hand_index)) {
                    return ActionApplicationResult::ILLEGAL_ACTION;
                }
                player.add_to_bankroll(player.get_hand(hand_index).get_bet() / 2);
                player.get_hand(hand_index).set_surrendered(true);
                state.surrendered = true;
                state.action_count++;
                return ActionApplicationResult::APPLIED_TURN_COMPLETE;
            }
        }

        std::unreachable();
    }

    [[nodiscard]] inline HandOutcome Game::determine_outcome(
        const Hand& player_hand,
        const Hand& dealer_hand
    ) const noexcept {
        bool was_split = (player_hand.get_origin() == HandOrigin::SPLIT);

        if (player_hand.is_surrendered()) {
            return HandOutcome::SURRENDER_LOSS;
        }

        if (player_hand.is_bust()) {
            return HandOutcome::PLAYER_BUST_LOSS;
        } else if (player_hand.is_blackjack() && !was_split) {
            if (dealer_hand.is_blackjack()) {
                return HandOutcome::PUSH;
            } else {
                return HandOutcome::BLACKJACK_WIN;
            }
        } else if (dealer_hand.is_bust()) {
            return HandOutcome::DEALER_BUST_WIN;
        } else if (dealer_hand.is_blackjack()) {
            return HandOutcome::DEALER_BLACKJACK_LOSS;
        } else {
            uint8_t player_value = player_hand.get_value();
            uint8_t dealer_value = dealer_hand.get_value();

            if (player_value > dealer_value) {
                return HandOutcome::REGULAR_WIN;
            } else if (player_value < dealer_value) {
                return HandOutcome::DEALER_WIN_LOSS;
            } else {
                return HandOutcome::PUSH;
            }
        }
    }

    void Game::resolve_hand(Player& player, uint8_t hand_index) noexcept {
        const Hand& player_hand = player.get_hand(hand_index);
        const Hand& dealer_hand = this->dealer.get_hand(0);

        HandOutcome outcome = this->determine_outcome(player_hand, dealer_hand);

        uint32_t payout = this->calculate_payout(outcome, player_hand.get_bet());
        if (payout > 0) {
            player.add_to_bankroll(payout);
        }

        this->hands_played_count++;
    }

    [[nodiscard]] inline uint32_t Game::calculate_payout(HandOutcome outcome, uint32_t bet) const noexcept {
        switch (outcome) {
            case HandOutcome::BLACKJACK_WIN:
                return this->betting_config.calculate_blackjack_payout(bet);

            case HandOutcome::REGULAR_WIN:
            case HandOutcome::DEALER_BUST_WIN:
                return bet + bet;

            case HandOutcome::PUSH:
                return bet;

            case HandOutcome::PLAYER_BUST_LOSS:
            case HandOutcome::DEALER_WIN_LOSS:
            case HandOutcome::DEALER_BLACKJACK_LOSS:
            case HandOutcome::SURRENDER_LOSS:
                return 0;
        }
    }

    void Game::resolve_all_hands() noexcept {
        for (uint8_t p = 0; p < this->player_count; p++) {
            Player& player = this->players[p];
            for (uint8_t h = 0; h < player.get_active_hand_count(); h++) {
                this->resolve_hand(player, h);
            }
        }
    }

    [[nodiscard]] inline const BettingConfig& Game::get_betting_config() const noexcept {
        return this->betting_config;
    }

    [[nodiscard]] inline Player& Game::get_player(uint8_t index) noexcept {
        return this->players[index];
    }

    [[nodiscard]] inline const Player& Game::get_dealer() const noexcept {
        return this->dealer;
    }

    [[nodiscard]] inline Player& Game::get_dealer() noexcept {
        return this->dealer;
    }

    [[nodiscard]] inline GameStatistics Game::aggregate_statistics() const noexcept {
        uint64_t total_ending = 0;
        for (uint8_t i = 0; i < this->player_count; i++) {
            total_ending += this->players[i].get_bankroll();
        }
        return GameStatistics(
            this->hands_played_count,
            this->starting_bankroll_total,
            static_cast<uint32_t>(total_ending)
        );
    }

    inline ActionApplicationResult Game::apply_dealer_decision(Decision decision) noexcept {
        switch (decision) {
            case Decision::HIT:
                this->dealer.add_card_to_hand(0, this->shoe.draw());
                this->dealer_hand_state.action_count++;
                return this->is_dealer_turn_complete()
                    ? ActionApplicationResult::APPLIED_TURN_COMPLETE
                    : ActionApplicationResult::APPLIED_CONTINUE;

            case Decision::STAND:
                this->dealer_hand_state.stood = true;
                this->dealer_hand_state.action_count++;
                return ActionApplicationResult::APPLIED_TURN_COMPLETE;

            case Decision::DOUBLE:
            case Decision::SPLIT:
            case Decision::SURRENDER:
                return ActionApplicationResult::ILLEGAL_ACTION;
        }

        std::unreachable();
    }

    void Game::play_turn_for_player(uint8_t player_index, uint8_t hand_index) noexcept {
        static DealerStrategy fallback;

        Player& player = this->players[player_index];
        PlayerStrategy* strategy = player.get_strategy();
        PlayerStrategy* effective = strategy ? strategy : &fallback;

        while (!this->is_hand_turn_complete(player_index, hand_index)) {
            const Card& upcard = this->dealer.get_hand(0).get_cards_data()[1];
            GameContext ctx(
                player.get_hand(hand_index),
                upcard,
                this->make_legal_actions(player_index, hand_index)
            );
            ActionApplicationResult result = this->apply_decision(
                player_index,
                hand_index,
                effective->get_decision(ctx)
            );

            if (result == ActionApplicationResult::APPLIED_TURN_COMPLETE) {
                break;
            }
        }
    }

    void Game::play_dealer_turn() noexcept {
        DealerStrategy dealer_strategy;
        const Card& upcard = this->dealer.get_hand(0).get_cards_data()[1];

        while (!this->is_dealer_turn_complete()) {
            GameContext ctx(this->dealer.get_hand(0), upcard, LegalActions::none());
            ActionApplicationResult result = this->apply_dealer_decision(dealer_strategy.get_decision(ctx));
            if (result == ActionApplicationResult::APPLIED_TURN_COMPLETE) {
                break;
            }
        }
    }

    [[nodiscard]] inline HandRoundState& Game::get_player_hand_state(uint8_t player_index, uint8_t hand_index) noexcept {
        return this->player_hand_states[player_index][hand_index];
    }

    [[nodiscard]] inline const HandRoundState& Game::get_player_hand_state(uint8_t player_index, uint8_t hand_index) const noexcept {
        return this->player_hand_states[player_index][hand_index];
    }

    [[nodiscard]] inline bool Game::is_first_action(uint8_t player_index, uint8_t hand_index) const noexcept {
        return this->get_player_hand_state(player_index, hand_index).action_count == 0;
    }

    [[nodiscard]] inline bool Game::can_double(uint8_t player_index, uint8_t hand_index) const noexcept {
        const Player& player = this->players[player_index];
        const Hand& hand = player.get_hand(hand_index);

        return this->is_first_action(player_index, hand_index)
            && hand.card_count() == 2
            && hand.get_bet() > 0
            && player.has_sufficient_funds(hand.get_bet());
    }

    [[nodiscard]] inline bool Game::can_split(uint8_t player_index, uint8_t hand_index) const noexcept {
        const Player& player = this->players[player_index];
        const Hand& hand = player.get_hand(hand_index);
        if (hand.card_count() != 2 || hand.get_bet() == 0) {
            return false;
        }

        const Card* cards = hand.get_cards_data();

        return player.get_active_hand_count() < Player::MAX_POSSIBLE_HANDS
            && player.has_sufficient_funds(hand.get_bet())
            && cards[0].get_rank() == cards[1].get_rank();
    }

    [[nodiscard]] inline bool Game::can_surrender(uint8_t player_index, uint8_t hand_index) const noexcept {
        const Hand& hand = this->players[player_index].get_hand(hand_index);

        return this->is_first_action(player_index, hand_index)
            && hand.card_count() == 2
            && hand.get_bet() > 0;
    }

    [[nodiscard]] inline bool Game::is_hand_turn_complete(uint8_t player_index, uint8_t hand_index) const noexcept {
        const Hand& hand = this->players[player_index].get_hand(hand_index);
        const HandRoundState& state = this->get_player_hand_state(player_index, hand_index);

        return hand.is_bust()
            || hand.is_blackjack()
            || state.stood
            || state.surrendered
            || state.doubled;
    }

    [[nodiscard]] inline bool Game::is_dealer_turn_complete() const noexcept {
        const Hand& dealer_hand = this->dealer.get_hand(0);
        return dealer_hand.is_bust()
            || dealer_hand.is_blackjack()
            || this->dealer_hand_state.stood;
    }

    [[nodiscard]] inline LegalActions Game::make_legal_actions(
        uint8_t player_index,
        uint8_t hand_index
    ) const noexcept {
        return LegalActions{
            this->can_double(player_index, hand_index),
            this->can_split(player_index, hand_index),
            this->can_surrender(player_index, hand_index),
        };
    }

    void Game::play_round(uint64_t seed) noexcept {
        this->shoe.shuffle(seed);

        for (uint8_t i = 0; i < this->player_count; i++) {
            this->players[i].set_active_hand_count(1);
            for (uint8_t h = 0; h < Player::MAX_POSSIBLE_HANDS; h++) {
                this->players[i].clear_hand(h);
                this->player_hand_states[i][h].reset();
            }
        }

        this->dealer.clear_hand(0);
        this->dealer_hand_state.reset();

        for (uint8_t i = 0; i < this->player_count; i++) {
            bool bet_placed = this->players[i].place_bet(this->betting_config.get_min_bet(), this->betting_config);
            if (!bet_placed) {
                this->players[i].set_active_hand_count(0);
            }
        }

        for (uint8_t deal_round = 0; deal_round < 2; deal_round++) {
            for (uint8_t i = 0; i < this->player_count; i++) {
                if (this->players[i].get_active_hand_count() > 0) {
                    this->players[i].add_card_to_hand(0, this->shoe.draw());
                }
            }

            this->dealer.add_card_to_hand(0, this->shoe.draw());
        }

        for (uint8_t i = 0; i < this->player_count; i++) {
            for (uint8_t h = 0; h < this->players[i].get_active_hand_count(); h++) {
                if (!this->players[i].get_hand(h).is_blackjack()) {
                    this->play_turn_for_player(i, h);
                }
            }
        }

        if (!this->dealer.get_hand(0).is_blackjack()) {
            this->play_dealer_turn();
        }

        this->resolve_all_hands();
    }
}

#endif
