#ifndef PLAYER_DEALER_HPP
#define PLAYER_DEALER_HPP

#include "player/strategy.hpp"

#include <cstdint>

class DealerStrategy : public PlayerStrategy {
private:
    static constexpr uint8_t DEALER_STAND_VALUE = 17;

public:
    Decision get_decision(const GameContext& context) const noexcept override;
};

#endif
