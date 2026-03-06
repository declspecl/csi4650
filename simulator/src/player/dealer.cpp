#include "player/dealer.hpp"

Decision DealerStrategy::get_decision(const GameContext& context) const noexcept {
    return (context.get_own_hand().get_value() < DEALER_STAND_VALUE)
        ? Decision::HIT
        : Decision::STAND;
}
