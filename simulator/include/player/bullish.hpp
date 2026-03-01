#ifndef PLAYER_BULLISH_HPP
#define PLAYER_BULLISH_HPP

#include "player/strategy.hpp"

#include <cstdint>

/**
 * This strategy hits until they cannot anymore
 */
class BullishStrategy : public PlayerStrategy {
private:
    static constexpr uint8_t HIT_UNTIL_VALUE = 21;

public:
    Decision get_decision(const GameContext& context) const noexcept override;
};

#endif