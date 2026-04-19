#ifndef BLACKJACK_HAND_OUTCOME_HPP
#define BLACKJACK_HAND_OUTCOME_HPP

#include <cstdint>

namespace blackjack::hand {
    enum class HandOutcome : uint8_t {
        BLACKJACK_WIN,
        REGULAR_WIN,
        DEALER_BUST_WIN,

        PLAYER_BUST_LOSS,
        DEALER_WIN_LOSS,
        DEALER_BLACKJACK_LOSS,
        SURRENDER_LOSS,

        PUSH,
    };
}

#endif
