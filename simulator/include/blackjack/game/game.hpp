#ifndef BLACKJACK_GAME_HPP
#define BLACKJACK_GAME_HPP

#include <blackjack/player/player.hpp>
#include <blackjack/deck/shoe.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/game/game_statistics.hpp>
#include <blackjack/hand/hand_origin.hpp>
#include <blackjack/hand/hand_outcome.hpp>
#include <blackjack/player/strategy/dealer.hpp>

#include <array>
#include <cassert>
#include <cstdint>

using blackjack::player::Player;
using blackjack::deck::Shoe;
using blackjack::hand::Hand;
using blackjack::hand::HandOutcome;
using blackjack::hand::HandOrigin;

namespace blackjack::game {
    class Game {
    public:
        static constexpr uint8_t MAX_NON_DEALER_PLAYERS = 7;

    private:
        std::array<Player, MAX_NON_DEALER_PLAYERS> players;
        Shoe shoe;
        Player dealer;
        uint8_t player_count;
        BettingConfig betting_config;
        uint64_t hands_played_count;
        uint32_t starting_bankroll_total;

    public:
        inline Game() noexcept;
        explicit inline Game(const BettingConfig& config) noexcept;

        inline void initialize_round() noexcept;
        inline void play_round(uint64_t seed) noexcept;
        inline void resolve_hand(Player& player, uint8_t hand_index) noexcept;
        inline void resolve_all_hands() noexcept;

        [[nodiscard]] inline HandOutcome determine_outcome(
            const Hand& player_hand,
            const Hand& dealer_hand
        ) const noexcept;

        [[nodiscard]] inline uint32_t calculate_payout(
            HandOutcome outcome,
            uint32_t bet
        ) const noexcept;

        [[nodiscard]] inline const BettingConfig& get_betting_config() const noexcept;
        [[nodiscard]] inline Player& get_player(uint8_t index) noexcept;
        [[nodiscard]] inline const Player& get_dealer() const noexcept;
        [[nodiscard]] inline GameStatistics aggregate_statistics() const noexcept;

    private:
        inline void play_turn_for_player(Player& player, uint8_t hand_index) noexcept;
        inline void play_dealer_turn() noexcept;
    };
}


namespace blackjack::game {
    Game::Game() noexcept
        : players({})
        , shoe()
        , dealer()
        , player_count(MAX_NON_DEALER_PLAYERS)
        , betting_config()
        , hands_played_count(0)
        , starting_bankroll_total(0)
    {}

    Game::Game(const BettingConfig& config) noexcept
        : players({})
        , shoe()
        , dealer()
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

    [[nodiscard]] inline HandOutcome Game::determine_outcome(
        const Hand& player_hand,
        const Hand& dealer_hand
    ) const noexcept {
        bool was_split = (player_hand.get_origin() == HandOrigin::SPLIT);

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
        assert(index < this->player_count);
        return this->players[index];
    }

    [[nodiscard]] inline const Player& Game::get_dealer() const noexcept {
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

    void Game::play_turn_for_player(Player& player, uint8_t hand_index) noexcept {
        using blackjack::player::strategy::DealerStrategy;
        using blackjack::player::strategy::Decision;
        using blackjack::player::strategy::GameContext;

        DealerStrategy fallback;
        blackjack::player::strategy::PlayerStrategy* strategy = player.get_strategy();
        blackjack::player::strategy::PlayerStrategy* effective = strategy ? strategy : &fallback;

        const Card& upcard = this->dealer.get_hand(0).get_cards_data()[1];

        while (!player.get_hand(hand_index).is_bust()) {
            GameContext ctx(player.get_hand(hand_index), upcard);
            Decision decision = effective->get_decision(ctx);

            if (decision == Decision::HIT) {
                player.add_card_to_hand(hand_index, this->shoe.draw());
            } else {
                break;
            }
        }
    }

    void Game::play_dealer_turn() noexcept {
        using blackjack::player::strategy::DealerStrategy;
        using blackjack::player::strategy::Decision;
        using blackjack::player::strategy::GameContext;

        DealerStrategy dealer_strategy;
        const Card& upcard = this->dealer.get_hand(0).get_cards_data()[1];

        while (!this->dealer.get_hand(0).is_bust()) {
            GameContext ctx(this->dealer.get_hand(0), upcard);
            Decision decision = dealer_strategy.get_decision(ctx);

            if (decision == Decision::HIT) {
                this->dealer.add_card_to_hand(0, this->shoe.draw());
            } else {
                break;
            }
        }
    }

    void Game::play_round(uint64_t seed) noexcept {
        this->shoe.shuffle(seed);

        for (uint8_t i = 0; i < this->player_count; i++) {
            this->players[i].clear_hand(0);
            this->players[i].set_active_hand_count(1);
        }
        this->dealer.clear_hand(0);

        for (uint8_t i = 0; i < this->player_count; i++) {
            bool bet_placed = this->players[i].place_bet(this->betting_config.get_min_bet(), this->betting_config);
            if (!bet_placed) {
                this->players[i].set_active_hand_count(0);
            }
        }

        for (uint8_t deal_round = 0; deal_round < 2; deal_round++) {
            for (uint8_t i = 0; i < this->player_count; i++) {
                this->players[i].add_card_to_hand(0, this->shoe.draw());
            }
            this->dealer.add_card_to_hand(0, this->shoe.draw());
        }

        for (uint8_t i = 0; i < this->player_count; i++) {
            if (this->players[i].get_active_hand_count() > 0
                    && !this->players[i].get_hand(0).is_blackjack()) {
                this->play_turn_for_player(this->players[i], 0);
            }
        }

        if (!this->dealer.get_hand(0).is_blackjack()) {
            this->play_dealer_turn();
        }

        this->resolve_all_hands();
    }
}

#endif
