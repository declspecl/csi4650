#ifndef PLAYER_STRATEGY_HPP
#define PLAYER_STRATEGY_HPP

#include "hand.hpp"

enum class Decision : uint8_t {
    HIT,
    STAND,
    DOUBLE,
    SPLIT,
    SURRENDER,
};

class PlayerStrategy {
public:
    virtual ~PlayerStrategy() = default;

    virtual Decision get_decision(const Hand& hand) const noexcept = 0;
    // TODO: add method to get decision, with dealer upcard as context
    // TODO: add method to get decision, with ALL public context (dealer upcard, other players' upcards, etc.)
};

#endif