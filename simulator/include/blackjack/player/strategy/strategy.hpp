#ifndef BLACKJACK_PLAYER_STRATEGY_HPP
#define BLACKJACK_PLAYER_STRATEGY_HPP

#include <blackjack/card/card.hpp>
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

    public:
        GameContext(
            const Hand& own,
            const Card& upcard,
            LegalActions legal_actions
        ) noexcept
            : own_hand(own)
            , dealer_upcard(upcard)
            , legal(legal_actions)
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
    };

    class PlayerStrategy {
    public:
        virtual ~PlayerStrategy() = default;

        [[nodiscard]] virtual Decision get_decision(const GameContext& context) const noexcept = 0;
    };
}

#endif
