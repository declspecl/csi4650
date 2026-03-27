#ifndef BLACKJACK_RANK_HPP
#define BLACKJACK_RANK_HPP

#include <cstdint>

namespace blackjack::card {
    static constexpr uint8_t RANK_START_VALUE = 2;

    enum class Rank : uint8_t {
        TWO = RANK_START_VALUE,
        THREE,
        FOUR,
        FIVE,
        SIX,
        SEVEN,
        EIGHT,
        NINE,
        TEN,
        JACK,
        QUEEN,
        KING,
        ACE,
    };
}

#endif
