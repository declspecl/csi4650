#ifndef SHOE_HPP
#define SHOE_HPP

#include "deck.hpp"

#include <array>

constexpr uint8_t MAX_SHOE_SIZE = 6;

class Shoe {
private:
    std::array<Deck, MAX_SHOE_SIZE> decks;
    uint8_t decks_to_use_count;
    uint8_t current_deck_index;

public:
    constexpr Shoe() noexcept;
    explicit constexpr Shoe(uint8_t decks_to_use_count) noexcept;

public:
    constexpr Card draw() noexcept;
    constexpr bool is_empty() const noexcept;
};

constexpr Shoe::Shoe() noexcept
    : decks({})
    , decks_to_use_count(MAX_SHOE_SIZE)
    , current_deck_index(0)
{}

constexpr Shoe::Shoe(uint8_t decks_to_use_count) noexcept
    : decks({})
    , decks_to_use_count(decks_to_use_count)
    , current_deck_index(0)
{}

constexpr Card Shoe::draw() noexcept {
    if (this->decks[this->current_deck_index].is_empty()) {
        this->current_deck_index++;
    }

    assert(this->current_deck_index < this->decks_to_use_count);

    return this->decks[this->current_deck_index].draw();
}

constexpr bool Shoe::is_empty() const noexcept {
    return (this->current_deck_index >= this->decks_to_use_count)
        && this->decks[this->current_deck_index].is_empty();
}

#endif