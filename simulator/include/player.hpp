#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "hand.hpp"
#include "player/strategy.hpp"
#include "betting_config.hpp"

#include <array>
#include <memory>

constexpr uint8_t MAX_NAME_LENGTH = 19;

/**
 * Max possible hands is 4:
 * - Initial hand (ex. A, A)
 * - Split hands and get same card (ex. A A, A A)
 * - Split both hands again (A, A, A, A)
 */
constexpr uint8_t MAX_POSSIBLE_HANDS = 4;

class Player {
private:
    std::array<Hand, MAX_POSSIBLE_HANDS> hands;
    std::array<unsigned char, MAX_NAME_LENGTH + sizeof('\0')> name;
    std::unique_ptr<PlayerStrategy> strategy; // TODO: may have to re-evaluate if we should use polymorphism here, may be a problem for parallelism (can't trivially copy players across threads)
    uint8_t active_hand_count;
    uint32_t bankroll_cents;

public:
    constexpr Player() noexcept;
    explicit constexpr Player(
        std::array<unsigned char, MAX_NAME_LENGTH + sizeof('\0')>&& name,
        std::unique_ptr<PlayerStrategy> strategy
    ) noexcept;

    constexpr void initialize_bankroll(uint32_t initial_amount) noexcept;
    constexpr uint32_t get_bankroll() const noexcept;
    constexpr bool has_sufficient_funds(uint32_t amount) const noexcept;
    constexpr void deduct_from_bankroll(uint32_t amount) noexcept;
    constexpr void add_to_bankroll(uint32_t amount) noexcept;

    constexpr bool place_bet(uint32_t bet_amount, const BettingConfig& config) noexcept;

    constexpr Hand& get_hand(uint8_t index) noexcept;
    constexpr uint8_t get_active_hand_count() const noexcept;
    constexpr void set_active_hand_count(uint8_t count) noexcept;
};

constexpr Player::Player() noexcept
    : hands({})
    , name("")
    , strategy(nullptr)
    , active_hand_count(1)
    , bankroll_cents(0)
{}

constexpr Player::Player(
    std::array<unsigned char, MAX_NAME_LENGTH + sizeof('\0')>&& name,
    std::unique_ptr<PlayerStrategy> strategy
) noexcept
    : hands({})
    , name(std::move(name))
    , strategy(std::move(strategy))
    , active_hand_count(1)
    , bankroll_cents(0)
{}

constexpr void Player::initialize_bankroll(uint32_t initial_amount) noexcept {
    this->bankroll_cents = initial_amount;
}

constexpr uint32_t Player::get_bankroll() const noexcept {
    return this->bankroll_cents;
}

constexpr bool Player::has_sufficient_funds(uint32_t amount) const noexcept {
    return this->bankroll_cents >= amount;
}

constexpr void Player::deduct_from_bankroll(uint32_t amount) noexcept {
    assert(this->bankroll_cents >= amount);
    this->bankroll_cents -= amount;
}

constexpr void Player::add_to_bankroll(uint32_t amount) noexcept {
    this->bankroll_cents += amount;
}

constexpr bool Player::place_bet(uint32_t bet_amount, const BettingConfig& config) noexcept {
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

constexpr Hand& Player::get_hand(uint8_t index) noexcept {
    assert(index < MAX_POSSIBLE_HANDS);
    return this->hands[index];
}

constexpr uint8_t Player::get_active_hand_count() const noexcept {
    return this->active_hand_count;
}

constexpr void Player::set_active_hand_count(uint8_t count) noexcept {
    assert(count <= MAX_POSSIBLE_HANDS);
    this->active_hand_count = count;
}

#endif
