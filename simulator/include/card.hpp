#ifndef CARD_HPP
#define CARD_HPP

#include <cstdint>

enum class Suit : uint8_t {
    SPADES,
    HEARTS,
    CLUBS,
    DIAMONDS,
};

constexpr uint8_t RANK_START_VALUE = 2;

enum class Rank : uint8_t {
    TWO = RANK_START_VALUE, // to enable simple cast to true value
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

class Card {
private:
    Suit suit;
    Rank rank;

public:
    constexpr Card() noexcept;
    explicit constexpr Card(Suit suit, Rank rank) noexcept;

public:
    constexpr Suit get_suit() const noexcept;
    constexpr Rank get_rank() const noexcept;
    constexpr uint8_t get_max_value() const noexcept;
};

constexpr Card::Card() noexcept
    : suit(Suit::SPADES)
    , rank(Rank::TWO)
{}

constexpr Card::Card(Suit suit, Rank rank) noexcept
    : suit(suit)
    , rank(rank)
{}

constexpr Suit Card::get_suit() const noexcept {
    return suit;
}

constexpr Rank Card::get_rank() const noexcept {
    return rank;
}

constexpr uint8_t Card::get_max_value() const noexcept {
    switch (rank) {
        case Rank::JACK:
        case Rank::QUEEN:
        case Rank::KING:
            return 10;
        case Rank::ACE:
            return 11;
        default:
            return static_cast<uint8_t>(rank);
    }
}

#endif