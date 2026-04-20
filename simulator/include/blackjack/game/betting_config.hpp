#ifndef BLACKJACK_BETTING_CONFIG_HPP
#define BLACKJACK_BETTING_CONFIG_HPP

#include <cstdint>

namespace blackjack::game {
    class BettingConfig {
    private:
        uint32_t min_bet_cents;
        uint32_t max_bet_cents;
        uint32_t initial_bankroll_cents;

    public:
        static constexpr uint32_t DEFAULT_MIN_BET_CENTS = 100;
        static constexpr uint32_t DEFAULT_MAX_BET_CENTS = 10000;
        static constexpr uint32_t DEFAULT_INITIAL_BANKROLL_CENTS = 100000;

        inline BettingConfig() noexcept;
        explicit inline BettingConfig(
            uint32_t min_bet,
            uint32_t max_bet,
            uint32_t initial_bankroll
        ) noexcept;

        [[nodiscard]] uint32_t get_min_bet() const noexcept;
        [[nodiscard]] uint32_t get_max_bet() const noexcept;
        [[nodiscard]] uint32_t get_initial_bankroll() const noexcept;
        [[nodiscard]] bool is_valid_bet(uint32_t bet) const noexcept;
    };
}


namespace blackjack::game {
    BettingConfig::BettingConfig() noexcept
        : min_bet_cents(DEFAULT_MIN_BET_CENTS)
        , max_bet_cents(DEFAULT_MAX_BET_CENTS)
        , initial_bankroll_cents(DEFAULT_INITIAL_BANKROLL_CENTS)
    {}

    BettingConfig::BettingConfig(
        uint32_t min_bet,
        uint32_t max_bet,
        uint32_t initial_bankroll
    ) noexcept
        : min_bet_cents(min_bet)
        , max_bet_cents(max_bet)
        , initial_bankroll_cents(initial_bankroll)
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

    [[nodiscard]] inline bool BettingConfig::is_valid_bet(uint32_t bet) const noexcept {
        return bet >= this->min_bet_cents && bet <= this->max_bet_cents;
    }
}

#endif
