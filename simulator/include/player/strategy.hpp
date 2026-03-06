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

// TODO: Consider expanding GameContext to include full game state for more sophisticated decision-making
// (e.g., other players' hands, remaining deck composition, betting amounts, etc.)
class GameContext {
private:
    const Hand& own_hand;
    const Card& dealer_upcard;

public:
    constexpr GameContext(
        const Hand& own,
        const Card& upcard
    ) noexcept
        : own_hand(own)
        , dealer_upcard(upcard)
    {}

    constexpr const Hand& get_own_hand() const noexcept {
        return own_hand;
    }

    constexpr const Card& get_dealer_upcard() const noexcept {
        return dealer_upcard;
    }
};

class PlayerStrategy {
public:
    virtual ~PlayerStrategy() = default;

    virtual Decision get_decision(const GameContext& context) const noexcept = 0;
};

#endif
