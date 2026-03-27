#ifndef BLACKJACK_PLAYER_STRATEGY_BULLISH_HPP
#define BLACKJACK_PLAYER_STRATEGY_BULLISH_HPP

#include <blackjack/player/strategy/strategy.hpp>

#include <cstdint>

namespace blackjack::player::strategy {
    /**
     * This strategy hits until they cannot anymore
     */
    class BullishStrategy : public PlayerStrategy {
    private:
        static constexpr uint8_t HIT_UNTIL_VALUE = 21;

    public:
        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override;
    };

    inline Decision BullishStrategy::get_decision(const GameContext& context) const noexcept {
        return (context.get_own_hand().get_value() < this->HIT_UNTIL_VALUE)
            ? Decision::HIT
            : Decision::STAND;
    }
}

#endif
