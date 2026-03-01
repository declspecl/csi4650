#ifndef PLAYER_BEARISH_HPP
#define PLAYER_BEARISH_HPP

#include "player/strategy.hpp"

/**
 * This strategy hits until they could possibly bust (12), then stands
 */
class BearishStrategy : public PlayerStrategy {
private:
    static constexpr uint8_t HIT_UNTIL_VALUE = 12;

public:
    Decision get_decision(const GameContext& context) const noexcept override;
};

#endif