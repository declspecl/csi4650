#ifndef BLACKJACK_PLAYER_STRATEGY_SURRENDER_FIRST_HPP
#define BLACKJACK_PLAYER_STRATEGY_SURRENDER_FIRST_HPP

#include <blackjack/player/strategy/basic.hpp>
#include <blackjack/player/strategy/strategy.hpp>

namespace blackjack::player::strategy {
    class SurrenderFirstStrategy : public PlayerStrategy {
    private:
        BasicStrategy basic;

    public:
        constexpr SurrenderFirstStrategy() noexcept : basic(true) {}
        constexpr explicit SurrenderFirstStrategy(bool das_allowed) noexcept
            : basic(das_allowed) {}

        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override {
            if (context.can_surrender()) return Decision::SURRENDER;
            return this->basic.get_decision(context);
        }
    };
}

#endif
