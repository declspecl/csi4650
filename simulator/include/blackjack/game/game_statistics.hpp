#ifndef BLACKJACK_GAME_STATISTICS_HPP
#define BLACKJACK_GAME_STATISTICS_HPP

#include <cstdint>

namespace blackjack::game {
    class GameStatistics {
    private:
        uint64_t hands_played;
        uint32_t starting_bankroll_cents;
        uint32_t ending_bankroll_cents;

    public:
        explicit GameStatistics(
            uint64_t hands_played,
            uint32_t starting_bankroll,
            uint32_t ending_bankroll
        ) noexcept
            : hands_played(hands_played)
            , starting_bankroll_cents(starting_bankroll)
            , ending_bankroll_cents(ending_bankroll)
        {}

        [[nodiscard]] uint64_t get_hands_played() const noexcept {
            return this->hands_played;
        }

        [[nodiscard]] int64_t get_bankroll_delta() const noexcept {
            return static_cast<int64_t>(this->ending_bankroll_cents)
                 - static_cast<int64_t>(this->starting_bankroll_cents);
        }

        [[nodiscard]] double get_expected_value_per_hand() const noexcept {
            if (this->hands_played == 0) {
                return 0.0;
            }
            return static_cast<double>(this->get_bankroll_delta())
                 / static_cast<double>(this->hands_played);
        }

        [[nodiscard]] uint32_t get_starting_bankroll() const noexcept {
            return this->starting_bankroll_cents;
        }

        [[nodiscard]] uint32_t get_ending_bankroll() const noexcept {
            return this->ending_bankroll_cents;
        }
    };
}

#endif
