#ifndef BLACKJACK_PLAYER_STRATEGY_DEALER_HPP
#define BLACKJACK_PLAYER_STRATEGY_DEALER_HPP

#include <blackjack/player/strategy/strategy.hpp>

#include <cstdint>

namespace blackjack::player::strategy {
    class DealerStrategy : public PlayerStrategy {
    private:
        static constexpr uint8_t DEALER_STAND_VALUE = 17;

    public:
        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override;
    };

    inline Decision DealerStrategy::get_decision(const GameContext& context) const noexcept {
        return (context.get_own_hand().get_value() < this->DEALER_STAND_VALUE)
            ? Decision::HIT
            : Decision::STAND;
    }
}

#endif
