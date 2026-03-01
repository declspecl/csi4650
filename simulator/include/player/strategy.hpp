#ifndef PLAYER_STRATEGY_HPP
#define PLAYER_STRATEGY_HPP

#include "hand.hpp"
#include <cstdint>

enum class Decision : uint8_t {
    HIT,
    STAND,
    DOUBLE,
    SPLIT,
    SURRENDER,
};

struct GameContext {
    const Hand& own_hand;
    const Card& dealer_upcard;

    constexpr GameContext(
        const Hand& own,
        const Card& upcard
    ) noexcept
        : own_hand(own)
        , dealer_upcard(upcard)
    {}
};

class PlayerStrategy {
public:
    virtual ~PlayerStrategy() = default;

    virtual Decision get_decision(const GameContext& context) const noexcept = 0;
};

#endif
