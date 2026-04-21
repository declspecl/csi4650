#ifndef BLACKJACK_PLAYER_STRATEGY_DOUBLE_FIRST_HPP
#define BLACKJACK_PLAYER_STRATEGY_DOUBLE_FIRST_HPP

#include <blackjack/player/strategy/basic.hpp>
#include <blackjack/player/strategy/strategy.hpp>

namespace blackjack::player::strategy {
    class DoubleFirstStrategy : public PlayerStrategy {
    private:
        BasicStrategy basic;

    public:
        constexpr DoubleFirstStrategy() noexcept : basic(true) {}
        constexpr explicit DoubleFirstStrategy(bool das_allowed) noexcept
            : basic(das_allowed) {}

        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override {
            if (context.can_double()) return Decision::DOUBLE;
            return this->basic.get_decision(context);
        }
    };
}

#endif
