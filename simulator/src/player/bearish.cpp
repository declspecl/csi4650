#include "player/bearish.hpp"

Decision BearishStrategy::get_decision(const Hand& hand) const noexcept {
    return (hand.get_value() < HIT_UNTIL_VALUE)
        ? Decision::HIT
        : Decision::STAND;
}