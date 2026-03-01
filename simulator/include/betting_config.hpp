#ifndef BETTING_CONFIG_HPP
#define BETTING_CONFIG_HPP

#include <cstdint>

struct BettingConfig {
    uint32_t min_bet_cents;
    uint32_t max_bet_cents;
    uint32_t initial_bankroll_cents;

    uint8_t blackjack_payout_numerator;
    uint8_t blackjack_payout_denominator;

    constexpr BettingConfig() noexcept
        : min_bet_cents(100)
        , max_bet_cents(10000)
        , initial_bankroll_cents(100000)
        , blackjack_payout_numerator(3)
        , blackjack_payout_denominator(2)
    {}

    explicit constexpr BettingConfig(
        uint32_t min_bet,
        uint32_t max_bet,
        uint32_t initial_bankroll,
        uint8_t bj_payout_num = 3,
        uint8_t bj_payout_denom = 2
    ) noexcept
        : min_bet_cents(min_bet)
        , max_bet_cents(max_bet)
        , initial_bankroll_cents(initial_bankroll)
        , blackjack_payout_numerator(bj_payout_num)
        , blackjack_payout_denominator(bj_payout_denom)
    {}

    constexpr bool is_valid_bet(uint32_t bet_cents) const noexcept {
        return bet_cents >= min_bet_cents && bet_cents <= max_bet_cents;
    }

    constexpr uint32_t calculate_blackjack_payout(uint32_t bet_cents) const noexcept {
        return bet_cents + (bet_cents * blackjack_payout_numerator / blackjack_payout_denominator);
    }
};

#endif
