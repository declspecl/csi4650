#ifndef BLACKJACK_PLAYER_STRATEGY_HPP
#define BLACKJACK_PLAYER_STRATEGY_HPP

#include <blackjack/card/card.hpp>
#include <blackjack/game/betting_config.hpp>
#include <blackjack/hand/hand.hpp>

#include <cstdint>

using blackjack::card::Card;
using blackjack::hand::Hand;

namespace blackjack::player::strategy {
    enum class Decision : uint8_t {
        HIT,
        STAND,
        DOUBLE,
        SPLIT,
        SURRENDER,
    };

    struct LegalActions {
        bool can_double;
        bool can_split;
        bool can_surrender;

        static constexpr LegalActions none() noexcept {
            return LegalActions{false, false, false};
        }
    };

    class GameContext {
    private:
        const Hand& own_hand;
        const Card& dealer_upcard;
        LegalActions legal;
        int16_t running_count;
        uint16_t cards_remaining;

    public:
        GameContext(
            const Hand& own,
            const Card& upcard,
            LegalActions legal_actions,
            int16_t running_count = 0,
            uint16_t cards_remaining = 312
        ) noexcept
            : own_hand(own)
            , dealer_upcard(upcard)
            , legal(legal_actions)
            , running_count(running_count)
            , cards_remaining(cards_remaining)
        {}

        [[nodiscard]] const Hand& get_own_hand() const noexcept {
            return this->own_hand;
        }

        [[nodiscard]] const Card& get_dealer_upcard() const noexcept {
            return this->dealer_upcard;
        }

        [[nodiscard]] const LegalActions& get_legal_actions() const noexcept {
            return this->legal;
        }

        [[nodiscard]] bool can_double() const noexcept {
            return this->legal.can_double;
        }

        [[nodiscard]] bool can_split() const noexcept {
            return this->legal.can_split;
        }

        [[nodiscard]] bool can_surrender() const noexcept {
            return this->legal.can_surrender;
        }

        [[nodiscard]] int16_t get_running_count() const noexcept {
            return this->running_count;
        }

        [[nodiscard]] float get_true_count() const noexcept {
            if (this->cards_remaining == 0) return 0.0f;
            return static_cast<float>(this->running_count)
                 / (static_cast<float>(this->cards_remaining) / 52.0f);
        }
    };

    class PlayerStrategy {
    public:
        virtual ~PlayerStrategy() = default;

        [[nodiscard]] virtual Decision get_decision(const GameContext& context) const noexcept = 0;

        [[nodiscard]] virtual uint32_t get_bet_size(
            float true_count,
            const blackjack::game::BettingConfig& config
        ) const noexcept {
            (void)true_count;
            return config.get_min_bet();
        }
    };
}

#endif
