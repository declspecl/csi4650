#include "player/bullish.hpp"

Decision BullishStrategy::get_decision(const Hand& hand) const noexcept {
    return (hand.get_value() < HIT_UNTIL_VALUE)
        ? Decision::HIT
        : Decision::STAND;
}