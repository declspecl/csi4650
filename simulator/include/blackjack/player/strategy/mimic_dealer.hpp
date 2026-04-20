#ifndef BLACKJACK_PLAYER_STRATEGY_MIMIC_DEALER_HPP
#define BLACKJACK_PLAYER_STRATEGY_MIMIC_DEALER_HPP

#include <blackjack/player/strategy/strategy.hpp>

#include <cstdint>

namespace blackjack::player::strategy {
    /**
     * Plays a seated player like the dealer: hit until 17, optionally hitting
     * soft 17. Never doubles, splits, or surrenders.
     */
    class MimicDealerStrategy : public PlayerStrategy {
    private:
        static constexpr uint8_t STAND_VALUE = 17;
        bool hits_soft_17;

    public:
        constexpr MimicDealerStrategy() noexcept : hits_soft_17(false) {}
        constexpr explicit MimicDealerStrategy(bool hits_soft_17_) noexcept
            : hits_soft_17(hits_soft_17_) {}

        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override;
    };

    inline Decision MimicDealerStrategy::get_decision(const GameContext& context) const noexcept {
        const auto& hand = context.get_own_hand();
        uint8_t value = hand.get_value();

        if (value < STAND_VALUE) {
            return Decision::HIT;
        }
        if (value == STAND_VALUE && this->hits_soft_17 && hand.is_soft()) {
            return Decision::HIT;
        }
        return Decision::STAND;
    }
}

#endif
