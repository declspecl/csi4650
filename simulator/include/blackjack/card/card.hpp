#ifndef BLACKJACK_CARD_HPP
#define BLACKJACK_CARD_HPP

#include <blackjack/card/rank.hpp>
#include <blackjack/card/suit.hpp>

#include <cstdint>

namespace blackjack::card {
    class Card {
    private:
        Suit suit = Suit::SPADES;
        Rank rank = Rank::TWO;

    public:
        constexpr Card() noexcept = default;
        explicit constexpr Card(Suit suit, Rank rank) noexcept;

        [[nodiscard]] inline Suit get_suit() const noexcept;
        [[nodiscard]] inline Rank get_rank() const noexcept;
        [[nodiscard]] inline uint8_t get_max_value() const noexcept;
    };
}


namespace blackjack::card {
    constexpr Card::Card(Suit suit, Rank rank) noexcept
        : suit(suit)
        , rank(rank)
    {}

    Suit Card::get_suit() const noexcept {
        return this->suit;
    }

    Rank Card::get_rank() const noexcept {
        return this->rank;
    }

    uint8_t Card::get_max_value() const noexcept {
        switch (this->rank) {
            case Rank::JACK:
            case Rank::QUEEN:
            case Rank::KING:
                return 10;
            case Rank::ACE:
                return 11;
            default:
                return static_cast<uint8_t>(this->rank);
        }
    }
}

#endif
