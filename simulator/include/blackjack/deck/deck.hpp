#ifndef BLACKJACK_DECK_HPP
#define BLACKJACK_DECK_HPP

#include <blackjack/card/card.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <random>

using blackjack::card::Card;
using blackjack::card::Rank;
using blackjack::card::RANK_START_VALUE;
using blackjack::card::Suit;

namespace blackjack::deck::detail {
    static constexpr std::array<Card, 52> make_standard_deck() noexcept {
        std::array<Card, 52> deck{};
        for (uint8_t i = 0; i < 52; i++) {
            deck[i] = Card(
                static_cast<Suit>(i / 13),
                static_cast<Rank>(i % 13 + RANK_START_VALUE)
            );
        }

        return deck;
    }
}


namespace blackjack::deck {
    class Deck {
    public:
        static constexpr uint8_t DECK_SIZE = 52;

    private:
        static constexpr std::array<Card, DECK_SIZE> STANDARD_FULL_DECK = detail::make_standard_deck();

        std::array<Card, DECK_SIZE> cards;
        uint8_t cards_remaining;

    public:
        inline Deck() noexcept;

        [[nodiscard]] inline Card draw() noexcept;
        [[nodiscard]] inline bool is_empty() const noexcept;
        inline void shuffle(uint64_t seed) noexcept;
    };
}


namespace blackjack::deck {
    Deck::Deck() noexcept
        : cards(STANDARD_FULL_DECK)
        , cards_remaining(DECK_SIZE)
    {}

    Card Deck::draw() noexcept {
        assert(!this->is_empty());
        return this->cards[--this->cards_remaining];
    }

    bool Deck::is_empty() const noexcept {
        return this->cards_remaining == 0;
    }

    void Deck::shuffle(uint64_t seed) noexcept {
        std::mt19937_64 rng(seed);
        std::shuffle(this->cards.begin(), this->cards.end(), rng);
        this->cards_remaining = DECK_SIZE;
    }
}

#endif
