#ifndef BLACKJACK_HAND_HPP
#define BLACKJACK_HAND_HPP

#include <blackjack/card/card.hpp>
#include <blackjack/hand/hand_origin.hpp>

#include <array>
#include <cassert>
#include <cstdint>

using blackjack::card::Card;
using blackjack::card::Rank;

namespace blackjack::hand {
    /**
     * Max hand size is 11 cards:
     * - Four aces (value of 4)
     * - Four twos (value of 8)
     * - Three threes (value of 9)
     * - 4 + 8 + 9 = 21
     */
    class Hand {
    public:
        static constexpr uint8_t MAX_HAND_SIZE = 11;

    private:
        std::array<Card, MAX_HAND_SIZE> cards;
        uint8_t cards_in_hand;
        uint32_t bet_cents;
        HandOrigin origin;

    public:
        inline Hand() noexcept;

        inline void add_card(const Card& card) noexcept;
        inline void clear() noexcept;

        inline void set_bet(uint32_t bet) noexcept;
        [[nodiscard]] inline uint32_t get_bet() const noexcept;

        inline void set_origin(HandOrigin new_origin) noexcept;
        [[nodiscard]] inline HandOrigin get_origin() const noexcept;

        [[nodiscard]] inline uint8_t get_value() const noexcept;
        [[nodiscard]] inline bool is_blackjack() const noexcept;
        [[nodiscard]] inline bool is_bust() const noexcept;
        [[nodiscard]] inline uint8_t card_count() const noexcept;
        [[nodiscard]] inline const Card* get_cards_data() const noexcept;
    };
}


namespace blackjack::hand {
    Hand::Hand() noexcept
        : cards()
        , cards_in_hand(0)
        , bet_cents(0)
        , origin(HandOrigin::NATURAL)
    {}

    void Hand::add_card(const Card& card) noexcept {
        assert(this->cards_in_hand < this->MAX_HAND_SIZE);
        this->cards[this->cards_in_hand++] = card;
    }

    [[nodiscard]] inline uint8_t Hand::get_value() const noexcept {
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

    void Hand::clear() noexcept {
        this->cards_in_hand = 0;
        this->bet_cents = 0;
        this->origin = HandOrigin::NATURAL;
    }

    void Hand::set_bet(uint32_t bet) noexcept {
        this->bet_cents = bet;
    }

    [[nodiscard]] inline uint32_t Hand::get_bet() const noexcept {
        return this->bet_cents;
    }

    void Hand::set_origin(HandOrigin new_origin) noexcept {
        this->origin = new_origin;
    }

    [[nodiscard]] inline HandOrigin Hand::get_origin() const noexcept {
        return this->origin;
    }

    [[nodiscard]] inline bool Hand::is_blackjack() const noexcept {
        return this->cards_in_hand == 2 && this->get_value() == 21;
    }

    [[nodiscard]] inline bool Hand::is_bust() const noexcept {
        return this->get_value() > 21;
    }

    [[nodiscard]] inline uint8_t Hand::card_count() const noexcept {
        return this->cards_in_hand;
    }

    [[nodiscard]] inline const Card* Hand::get_cards_data() const noexcept {
        return this->cards.data();
    }
}

#endif
