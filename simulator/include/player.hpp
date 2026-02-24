#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "hand.hpp"
#include "player/strategy.hpp"

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

public:
    constexpr Player() noexcept;
    explicit constexpr Player(
        std::array<unsigned char, MAX_NAME_LENGTH + sizeof('\0')>&& name,
        std::unique_ptr<PlayerStrategy> strategy
    ) noexcept;
};

constexpr Player::Player() noexcept
    : hands({})
    , name("")
    , strategy(nullptr)
    , active_hand_count(1)
{}

constexpr Player::Player(
    std::array<unsigned char, MAX_NAME_LENGTH + sizeof('\0')>&& name,
    std::unique_ptr<PlayerStrategy> strategy
) noexcept
    : hands({})
    , name(std::move(name))
    , strategy(std::move(strategy))
    , active_hand_count(1)
{}

#endif