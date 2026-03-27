#ifndef BLACKJACK_SUIT_HPP
#define BLACKJACK_SUIT_HPP

#include <cstdint>

namespace blackjack::card {
    enum class Suit : uint8_t {
        SPADES,
        HEARTS,
        CLUBS,
        DIAMONDS,
    };
}

#endif
