#ifndef BETTING_CONFIG_HPP
#define BETTING_CONFIG_HPP

#include <cstdint>

constexpr uint32_t DEFAULT_MIN_BET_CENTS = 100;
constexpr uint32_t DEFAULT_MAX_BET_CENTS = 10000;
constexpr uint32_t DEFAULT_INITIAL_BANKROLL_CENTS = 100000;
constexpr uint8_t DEFAULT_BLACKJACK_PAYOUT_NUMERATOR = 3;
constexpr uint8_t DEFAULT_BLACKJACK_PAYOUT_DENOMINATOR = 2;

class BettingConfig {
private:
    uint32_t min_bet_cents;
    uint32_t max_bet_cents;
    uint32_t initial_bankroll_cents;

    uint8_t blackjack_payout_numerator;
    uint8_t blackjack_payout_denominator;

public:
    constexpr BettingConfig() noexcept
        : min_bet_cents(DEFAULT_MIN_BET_CENTS)
        , max_bet_cents(DEFAULT_MAX_BET_CENTS)
        , initial_bankroll_cents(DEFAULT_INITIAL_BANKROLL_CENTS)
        , blackjack_payout_numerator(DEFAULT_BLACKJACK_PAYOUT_NUMERATOR)
        , blackjack_payout_denominator(DEFAULT_BLACKJACK_PAYOUT_DENOMINATOR)
    {}

    explicit constexpr BettingConfig(
        uint32_t min_bet,
        uint32_t max_bet,
        uint32_t initial_bankroll,
        uint8_t bj_payout_num = DEFAULT_BLACKJACK_PAYOUT_NUMERATOR,
        uint8_t bj_payout_denom = DEFAULT_BLACKJACK_PAYOUT_DENOMINATOR
    ) noexcept
        : min_bet_cents(min_bet)
        , max_bet_cents(max_bet)
        , initial_bankroll_cents(initial_bankroll)
        , blackjack_payout_numerator(bj_payout_num)
        , blackjack_payout_denominator(bj_payout_denom)
    {}

    constexpr uint32_t get_min_bet() const noexcept {
        return min_bet_cents;
    }

    constexpr uint32_t get_max_bet() const noexcept {
        return max_bet_cents;
    }

    constexpr uint32_t get_initial_bankroll() const noexcept {
        return initial_bankroll_cents;
    }

    constexpr uint8_t get_blackjack_payout_numerator() const noexcept {
        return blackjack_payout_numerator;
    }

    constexpr uint8_t get_blackjack_payout_denominator() const noexcept {
        return blackjack_payout_denominator;
    }

    constexpr bool is_valid_bet(uint32_t bet_cents) const noexcept {
        return bet_cents >= min_bet_cents && bet_cents <= max_bet_cents;
    }

    constexpr uint32_t calculate_blackjack_payout(uint32_t bet_cents) const noexcept {
        return bet_cents + (bet_cents * blackjack_payout_numerator / blackjack_payout_denominator);
    }
};

#endif
