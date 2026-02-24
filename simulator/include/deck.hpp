#ifndef DECK_HPP
#define DECK_HPP

#include "card.hpp"

#include <array>
#include <cassert>
#include <cstdint>

constexpr uint8_t DECK_SIZE = 52;

namespace internal {
    static constexpr std::array<Card, DECK_SIZE> make_standard_deck() noexcept {
        std::array<Card, DECK_SIZE> deck{};
        for (uint8_t i = 0; i < DECK_SIZE; i++) {
            deck[i] = Card(
                static_cast<Suit>(i / 13),
                static_cast<Rank>(i % 13 + RANK_START_VALUE)
            );
        }

        return deck;
    }
}

class Deck {
private:
    static constexpr std::array<Card, DECK_SIZE> STANDARD_FULL_DECK = internal::make_standard_deck();

    std::array<Card, DECK_SIZE> cards;
    uint8_t cards_remaining;

public:
    constexpr Deck() noexcept;

public:
    constexpr Card draw() noexcept;
    constexpr bool is_empty() const noexcept;
};

constexpr Deck::Deck() noexcept
    : cards(STANDARD_FULL_DECK)
    , cards_remaining(DECK_SIZE)
{}

constexpr Card Deck::draw() noexcept {
    assert(!this->is_empty());

    return this->cards[--this->cards_remaining];
}

constexpr bool Deck::is_empty() const noexcept {
    return this->cards_remaining == 0;
}

#endif