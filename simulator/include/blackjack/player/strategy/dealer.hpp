#ifndef BLACKJACK_PLAYER_STRATEGY_DEALER_HPP
#define BLACKJACK_PLAYER_STRATEGY_DEALER_HPP

#include <blackjack/player/strategy/strategy.hpp>

#include <cstdint>

namespace blackjack::player::strategy {
    class DealerStrategy : public PlayerStrategy {
    private:
        static constexpr uint8_t DEALER_STAND_VALUE = 17;
        bool hits_soft_17;

    public:
        constexpr DealerStrategy() noexcept : hits_soft_17(false) {}
        constexpr explicit DealerStrategy(bool hits_soft_17_) noexcept
            : hits_soft_17(hits_soft_17_) {}

        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override;
    };

    inline Decision DealerStrategy::get_decision(const GameContext& context) const noexcept {
        const auto& hand = context.get_own_hand();
        uint8_t value = hand.get_value();

        if (value < DEALER_STAND_VALUE) {
            return Decision::HIT;
        }
        if (value == DEALER_STAND_VALUE && this->hits_soft_17 && hand.is_soft()) {
            return Decision::HIT;
        }
        return Decision::STAND;
    }
}

#endif
