#ifndef BLACKJACK_PLAYER_STRATEGY_ALWAYS_STAND_HPP
#define BLACKJACK_PLAYER_STRATEGY_ALWAYS_STAND_HPP

#include <blackjack/player/strategy/strategy.hpp>

namespace blackjack::player::strategy {
    class AlwaysStandStrategy : public PlayerStrategy {
    public:
        [[nodiscard]] Decision get_decision(const GameContext&) const noexcept override {
            return Decision::STAND;
        }
    };
}

#endif
