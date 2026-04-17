#ifndef BLACKJACK_GAME_HAND_ROUND_STATE_HPP
#define BLACKJACK_GAME_HAND_ROUND_STATE_HPP

#include <cstdint>

namespace blackjack::game {
    /**
     * Represents the state of a player's hand at a point in time.
     * Differs from `Hand` in that it just tracks the state relevant to the game decision making, not the cards and such
     */
    struct HandRoundState {
        uint8_t action_count;
        bool stood;
        bool surrendered;

        HandRoundState() noexcept
            : action_count(0)
            , stood(false)
            , surrendered(false)
        {}

        void reset() noexcept {
            this->action_count = 0;
            this->stood = false;
            this->surrendered = false;
        }
    };
}

#endif
