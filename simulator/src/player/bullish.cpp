#include "player/bullish.hpp"

Decision BullishStrategy::get_decision(const GameContext& context) const noexcept {
    return (context.get_own_hand().get_value() < HIT_UNTIL_VALUE)
        ? Decision::HIT
        : Decision::STAND;
}