#ifndef BLACKJACK_SHOE_HPP
#define BLACKJACK_SHOE_HPP

#include <blackjack/deck/deck.hpp>

#include <array>
#include <cassert>
#include <cstdint>

using blackjack::card::Card;

namespace blackjack::deck {
    class Shoe {
    public:
        static constexpr uint8_t MAX_SHOE_SIZE = 6;

    private:
        std::array<Deck, MAX_SHOE_SIZE> decks;
        const uint8_t decks_to_use_count;
        uint8_t current_deck_index;

    public:
        inline Shoe() noexcept;
        explicit inline Shoe(uint8_t decks_to_use_count) noexcept;

        [[nodiscard]] inline Card draw() noexcept;
        [[nodiscard]] inline bool is_empty() const noexcept;
        [[nodiscard]] inline uint16_t cards_remaining() const noexcept;
        inline void shuffle(uint64_t seed) noexcept;
    };
}


namespace blackjack::deck {
    Shoe::Shoe() noexcept
        : decks({})
        , decks_to_use_count(MAX_SHOE_SIZE)
        , current_deck_index(0)
    {}

    Shoe::Shoe(uint8_t decks_to_use_count) noexcept
        : decks({})
        , decks_to_use_count(decks_to_use_count)
        , current_deck_index(0)
    {}

    Card Shoe::draw() noexcept {
        if (this->decks[this->current_deck_index].is_empty()) {
            this->current_deck_index++;
        }

        return this->decks[this->current_deck_index].draw();
    }

    bool Shoe::is_empty() const noexcept {
        return (this->current_deck_index >= this->decks_to_use_count)
            && this->decks[this->current_deck_index].is_empty();
    }

    uint16_t Shoe::cards_remaining() const noexcept {
        uint16_t total = 0;
        for (uint8_t i = this->current_deck_index; i < this->decks_to_use_count; i++) {
            total += this->decks[i].get_cards_remaining();
        }
        return total;
    }

    void Shoe::shuffle(uint64_t seed) noexcept {
        for (uint8_t i = 0; i < this->decks_to_use_count; i++) {
            this->decks[i].shuffle(seed + i);
        }
        this->current_deck_index = 0;
    }
}

#endif
