#include "player/bearish.hpp"

Decision BearishStrategy::get_decision(const GameContext& context) const noexcept {
    return (context.own_hand.get_value() < HIT_UNTIL_VALUE)
        ? Decision::HIT
        : Decision::STAND;
}