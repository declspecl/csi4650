#ifndef BLACKJACK_PLAYER_STRATEGY_HI_LO_HPP
#define BLACKJACK_PLAYER_STRATEGY_HI_LO_HPP

#include <blackjack/player/strategy/basic.hpp>
#include <blackjack/player/strategy/strategy.hpp>

#include <algorithm>
#include <cstdint>

namespace blackjack::player::strategy {
    class HiLoStrategy : public PlayerStrategy {
    private:
        BasicStrategy basic;

        static constexpr uint32_t MAX_BET_MULTIPLIER = 8;

    public:
        constexpr HiLoStrategy() noexcept : basic(true) {}
        constexpr explicit HiLoStrategy(bool das_allowed) noexcept
            : basic(das_allowed) {}

        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override;

        [[nodiscard]] uint32_t get_bet_size(
            float true_count,
            const blackjack::game::BettingConfig& config
        ) const noexcept override;
    };

    inline Decision HiLoStrategy::get_decision(const GameContext& context) const noexcept {
        const Hand& hand = context.get_own_hand();

        if (!hand.is_soft() && !hand.is_blackjack()) {
            float tc = context.get_true_count();
            uint8_t value = hand.get_value();
            uint8_t up = context.get_dealer_upcard().get_max_value();

            if (value == 16 && up == 10 && tc >= 0.0f) {
                return Decision::STAND;
            }
            if (value == 15 && up == 10 && tc >= 4.0f) {
                return context.can_surrender() ? Decision::SURRENDER : Decision::STAND;
            }
            if (value == 12 && up == 3 && tc >= 2.0f) {
                return Decision::STAND;
            }
            if (value == 12 && up == 2 && tc >= 3.0f) {
                return Decision::STAND;
            }
            if (value == 10 && up == 10 && tc >= 4.0f && context.can_double()) {
                return Decision::DOUBLE;
            }
            if (value == 10 && up == 11 && tc >= 4.0f && context.can_double()) {
                return Decision::DOUBLE;
            }
            if (value == 9 && up == 2 && tc >= 1.0f && context.can_double()) {
                return Decision::DOUBLE;
            }
            if (value == 9 && up == 7 && tc >= 3.0f && context.can_double()) {
                return Decision::DOUBLE;
            }
        }

        return this->basic.get_decision(context);
    }

    inline uint32_t HiLoStrategy::get_bet_size(
        float true_count,
        const blackjack::game::BettingConfig& config
    ) const noexcept {
        if (true_count <= 1.0f) {
            return config.get_min_bet();
        }
        uint32_t multiplier = std::min(
            static_cast<uint32_t>(true_count),
            MAX_BET_MULTIPLIER
        );
        uint32_t bet = config.get_min_bet() * multiplier;
        return std::min(bet, config.get_max_bet());
    }
}

#endif
