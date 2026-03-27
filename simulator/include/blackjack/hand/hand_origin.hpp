#ifndef BLACKJACK_HAND_ORIGIN_HPP
#define BLACKJACK_HAND_ORIGIN_HPP

#include <cstdint>

namespace blackjack::hand {
    enum class HandOrigin : uint8_t {
        NATURAL,
        SPLIT,
    };
}

#endif
