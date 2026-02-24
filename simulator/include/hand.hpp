#ifndef HAND_HPP
#define HAND_HPP

#include "card.hpp"

#include <array>
#include <cassert>
#include <cstdint>

/**
 * Max hand size is 11 cards:
 * - Four aces (value of 4)
 * - Four twos (value of 8)
 * - Three threes (value of 9)
 * - 4 + 8 + 9 = 21
 */
constexpr uint8_t MAX_HAND_SIZE = 11;

class Hand {
private:
    std::array<Card, MAX_HAND_SIZE> cards;
    uint8_t cards_in_hand;

public:
    constexpr Hand() noexcept;

public:
    constexpr void add_card(const Card& card) noexcept;
    constexpr uint8_t get_value() const noexcept;
    constexpr void clear() noexcept;
};

constexpr Hand::Hand() noexcept
    : cards()
    , cards_in_hand(0)
{}

constexpr void Hand::add_card(const Card& card) noexcept {
    assert(this->cards_in_hand < MAX_HAND_SIZE);

    this->cards[this->cards_in_hand++] = card;
}

constexpr uint8_t Hand::get_value() const noexcept {
    uint8_t value = 0;
    uint8_t aces_count = 0;
    for (uint8_t i = 0; i < this->cards_in_hand; i++) {
        value += this->cards[i].get_max_value();

        if (this->cards[i].get_rank() == Rank::ACE) {
            aces_count++;
        }
    }

    while (aces_count > 0 && value > 21) {
        value -= 10;
        aces_count--;
    }

    return value;
}

constexpr void Hand::clear() noexcept {
    this->cards_in_hand = 0;
}

#endif