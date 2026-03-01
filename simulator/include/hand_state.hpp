#ifndef HAND_STATE_HPP
#define HAND_STATE_HPP

#include <cstdint>

enum class HandState : uint8_t {
    PENDING = 0,
    SPLIT,
};

#endif
