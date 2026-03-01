#ifndef HAND_OUTCOME_HPP
#define HAND_OUTCOME_HPP

#include <cstdint>

enum class HandOutcome : uint8_t {
    BLACKJACK_WIN,
    REGULAR_WIN,
    DEALER_BUST_WIN,

    PLAYER_BUST_LOSS,
    DEALER_WIN_LOSS,
    DEALER_BLACKJACK_LOSS,

    PUSH,
};

#endif
