#ifndef BLACKJACK_BETTING_CONFIG_HPP
#define BLACKJACK_BETTING_CONFIG_HPP

#include <cstdint>

namespace blackjack::game {
    class BettingConfig {
    private:
        uint32_t min_bet_cents;
        uint32_t max_bet_cents;
        uint32_t initial_bankroll_cents;
        uint8_t blackjack_payout_numerator;
        uint8_t blackjack_payout_denominator;

    public:
        static constexpr uint32_t DEFAULT_MIN_BET_CENTS = 100;
        static constexpr uint32_t DEFAULT_MAX_BET_CENTS = 10000;
        static constexpr uint32_t DEFAULT_INITIAL_BANKROLL_CENTS = 100000;
        static constexpr uint8_t DEFAULT_BLACKJACK_PAYOUT_NUMERATOR = 3;
        static constexpr uint8_t DEFAULT_BLACKJACK_PAYOUT_DENOMINATOR = 2;

        inline BettingConfig() noexcept;
        explicit inline BettingConfig(
            uint32_t min_bet,
            uint32_t max_bet,
            uint32_t initial_bankroll,
            uint8_t bj_payout_num = DEFAULT_BLACKJACK_PAYOUT_NUMERATOR,
            uint8_t bj_payout_denom = DEFAULT_BLACKJACK_PAYOUT_DENOMINATOR
        ) noexcept;

        [[nodiscard]] uint32_t get_min_bet() const noexcept;
        [[nodiscard]] uint32_t get_max_bet() const noexcept;
        [[nodiscard]] uint32_t get_initial_bankroll() const noexcept;
        [[nodiscard]] uint8_t get_blackjack_payout_numerator() const noexcept;
        [[nodiscard]] uint8_t get_blackjack_payout_denominator() const noexcept;
        [[nodiscard]] bool is_valid_bet(uint32_t bet) const noexcept;
        [[nodiscard]] uint32_t calculate_blackjack_payout(uint32_t bet) const noexcept;
    };
}


namespace blackjack::game {
    BettingConfig::BettingConfig() noexcept
        : min_bet_cents(DEFAULT_MIN_BET_CENTS)
        , max_bet_cents(DEFAULT_MAX_BET_CENTS)
        , initial_bankroll_cents(DEFAULT_INITIAL_BANKROLL_CENTS)
        , blackjack_payout_numerator(DEFAULT_BLACKJACK_PAYOUT_NUMERATOR)
        , blackjack_payout_denominator(DEFAULT_BLACKJACK_PAYOUT_DENOMINATOR)
    {}

    BettingConfig::BettingConfig(
        uint32_t min_bet,
        uint32_t max_bet,
        uint32_t initial_bankroll,
        uint8_t bj_payout_num,
        uint8_t bj_payout_denom
    ) noexcept
        : min_bet_cents(min_bet)
        , max_bet_cents(max_bet)
        , initial_bankroll_cents(initial_bankroll)
        , blackjack_payout_numerator(bj_payout_num)
        , blackjack_payout_denominator(bj_payout_denom)
    {}

    [[nodiscard]] inline uint32_t BettingConfig::get_min_bet() const noexcept {
        return this->min_bet_cents;
    }

    [[nodiscard]] inline uint32_t BettingConfig::get_max_bet() const noexcept {
        return this->max_bet_cents;
    }

    [[nodiscard]] inline uint32_t BettingConfig::get_initial_bankroll() const noexcept {
        return this->initial_bankroll_cents;
    }

    [[nodiscard]] inline uint8_t BettingConfig::get_blackjack_payout_numerator() const noexcept {
        return this->blackjack_payout_numerator;
    }

    [[nodiscard]] inline uint8_t BettingConfig::get_blackjack_payout_denominator() const noexcept {
        return this->blackjack_payout_denominator;
    }

    [[nodiscard]] inline bool BettingConfig::is_valid_bet(uint32_t bet) const noexcept {
        return bet >= this->min_bet_cents && bet <= this->max_bet_cents;
    }

    [[nodiscard]] inline uint32_t BettingConfig::calculate_blackjack_payout(uint32_t bet) const noexcept {
        return bet + (bet * this->blackjack_payout_numerator / this->blackjack_payout_denominator);
    }
}

#endif
