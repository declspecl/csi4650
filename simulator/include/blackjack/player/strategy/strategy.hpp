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

    class GameContext {
    private:
        const Hand& own_hand;
        const Card& dealer_upcard;

    public:
        GameContext(
            const Hand& own,
            const Card& upcard
        ) noexcept
            : own_hand(own)
            , dealer_upcard(upcard)
        {}

        [[nodiscard]] const Hand& get_own_hand() const noexcept {
            return this->own_hand;
        }

        [[nodiscard]] const Card& get_dealer_upcard() const noexcept {
            return this->dealer_upcard;
        }
    };

    class PlayerStrategy {
    public:
        virtual ~PlayerStrategy() = default;

        [[nodiscard]] virtual Decision get_decision(const GameContext& context) const noexcept = 0;
    };
}

#endif
