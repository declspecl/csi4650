#ifndef BLACKJACK_PLAYER_HPP
#define BLACKJACK_PLAYER_HPP

#include <blackjack/hand/hand.hpp>
#include <blackjack/player/strategy/strategy.hpp>
#include <blackjack/game/betting_config.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <utility>

using blackjack::card::Card;
using blackjack::hand::Hand;
using blackjack::game::BettingConfig;
using blackjack::player::strategy::PlayerStrategy;

namespace blackjack::player {
    /**
     * Max possible hands is 4:
     * - Initial hand (ex. A, A)
     * - Split hands and get same card (ex. A A, A A)
     * - Split both hands again (A, A, A, A)
     */
    class Player {
    public:
        static constexpr uint8_t MAX_NAME_LENGTH = 19;
        static constexpr uint8_t MAX_POSSIBLE_HANDS = 4;

    private:
        std::array<Hand, MAX_POSSIBLE_HANDS> hands;
        std::array<char, MAX_NAME_LENGTH + 1> name;
        std::unique_ptr<PlayerStrategy> player_strategy;
        uint8_t active_hand_count;
        uint32_t bankroll_cents;

    public:
        inline Player() noexcept;
        explicit inline Player(
            std::array<char, MAX_NAME_LENGTH + 1>&& name,
            std::unique_ptr<PlayerStrategy> player_strategy
        ) noexcept;

        inline void initialize_bankroll(uint32_t initial_amount) noexcept;
        [[nodiscard]] inline uint32_t get_bankroll() const noexcept;
        [[nodiscard]] inline bool has_sufficient_funds(uint32_t amount) const noexcept;
        inline void deduct_from_bankroll(uint32_t amount) noexcept;
        inline void add_to_bankroll(uint32_t amount) noexcept;

        [[nodiscard]] inline bool place_bet(uint32_t bet_amount, const BettingConfig& config) noexcept;

        inline void clear_hand(uint8_t index) noexcept;
        inline void add_card_to_hand(uint8_t index, const Card& card) noexcept;
        inline void set_hand_bet(uint8_t index, uint32_t bet) noexcept;

        [[nodiscard]] inline const Hand& get_hand(uint8_t index) const noexcept;
        [[nodiscard]] inline uint8_t get_active_hand_count() const noexcept;
        inline void set_active_hand_count(uint8_t count) noexcept;
    };
}


namespace blackjack::player {
    Player::Player() noexcept
        : hands({})
        , name({})
        , player_strategy(nullptr)
        , active_hand_count(1)
        , bankroll_cents(0)
    {}

    Player::Player(
        std::array<char, MAX_NAME_LENGTH + 1>&& name,
        std::unique_ptr<PlayerStrategy> player_strategy
    ) noexcept
        : hands({})
        , name(std::move(name))
        , player_strategy(std::move(player_strategy))
        , active_hand_count(1)
        , bankroll_cents(0)
    {}

    void Player::initialize_bankroll(uint32_t initial_amount) noexcept {
        this->bankroll_cents = initial_amount;
    }

    [[nodiscard]] inline uint32_t Player::get_bankroll() const noexcept {
        return this->bankroll_cents;
    }

    [[nodiscard]] inline bool Player::has_sufficient_funds(uint32_t amount) const noexcept {
        return this->bankroll_cents >= amount;
    }

    void Player::deduct_from_bankroll(uint32_t amount) noexcept {
        this->bankroll_cents -= amount;
    }

    void Player::add_to_bankroll(uint32_t amount) noexcept {
        this->bankroll_cents += amount;
    }

    [[nodiscard]] inline bool Player::place_bet(uint32_t bet_amount, const BettingConfig& config) noexcept {
        if (!config.is_valid_bet(bet_amount)) {
            return false;
        }
        if (!this->has_sufficient_funds(bet_amount)) {
            return false;
        }

        this->deduct_from_bankroll(bet_amount);
        this->hands[0].set_bet(bet_amount);
        return true;
    }

    void Player::clear_hand(uint8_t index) noexcept {
        this->hands[index].clear();
    }

    void Player::add_card_to_hand(uint8_t index, const Card& card) noexcept {
        this->hands[index].add_card(card);
    }

    void Player::set_hand_bet(uint8_t index, uint32_t bet) noexcept {
        this->hands[index].set_bet(bet);
    }

    [[nodiscard]] inline const Hand& Player::get_hand(uint8_t index) const noexcept {
        return this->hands[index];
    }

    [[nodiscard]] inline uint8_t Player::get_active_hand_count() const noexcept {
        return this->active_hand_count;
    }

    void Player::set_active_hand_count(uint8_t count) noexcept {
        this->active_hand_count = count;
    }
}

#endif
