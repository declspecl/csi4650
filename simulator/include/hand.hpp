#ifndef HAND_HPP
#define HAND_HPP

#include "card.hpp"
#include "hand_state.hpp"

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
    uint32_t bet_cents;
    HandState state;

public:
    constexpr Hand() noexcept;

public:
    constexpr void add_card(const Card& card) noexcept;
    constexpr uint8_t get_value() const noexcept;
    constexpr void clear() noexcept;

    constexpr void set_bet(uint32_t bet) noexcept;
    constexpr uint32_t get_bet() const noexcept;

    constexpr void set_state(HandState new_state) noexcept;
    constexpr HandState get_state() const noexcept;

    constexpr bool is_blackjack() const noexcept;
    constexpr bool is_bust() const noexcept;
    constexpr uint8_t card_count() const noexcept;

    constexpr const Card* get_cards_data() const noexcept;
};
//
constexpr Hand::Hand() noexcept
    : cards()
    , cards_in_hand(0)
    , bet_cents(0)
    , state(HandState::PENDING)
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
    this->bet_cents = 0;
    this->state = HandState::PENDING;
}

constexpr void Hand::set_bet(uint32_t bet) noexcept {
    this->bet_cents = bet;
}

constexpr uint32_t Hand::get_bet() const noexcept {
    return this->bet_cents;
}

constexpr void Hand::set_state(HandState new_state) noexcept {
    this->state = new_state;
}

constexpr HandState Hand::get_state() const noexcept {
    return this->state;
}

constexpr bool Hand::is_blackjack() const noexcept {
    return this->cards_in_hand == 2 && this->get_value() == 21;
}

constexpr bool Hand::is_bust() const noexcept {
    return this->get_value() > 21;
}

constexpr uint8_t Hand::card_count() const noexcept {
    return this->cards_in_hand;
}

constexpr const Card* Hand::get_cards_data() const noexcept {
    return this->cards.data();
}

#endif
