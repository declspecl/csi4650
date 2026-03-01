#ifndef GAME_STATISTICS_HPP
#define GAME_STATISTICS_HPP

#include <cstdint>

class GameStatistics {
private:
    uint64_t hands_played;
    uint32_t starting_bankroll_cents;
    uint32_t ending_bankroll_cents;

public:
    constexpr GameStatistics() noexcept
        : hands_played(0)
        , starting_bankroll_cents(0)
        , ending_bankroll_cents(0)
    {}

    constexpr void set_starting_bankroll(uint32_t bankroll) noexcept {
        starting_bankroll_cents = bankroll;
    }

    constexpr void set_ending_bankroll(uint32_t bankroll) noexcept {
        ending_bankroll_cents = bankroll;
    }

    constexpr void increment_hands_played() noexcept {
        hands_played++;
    }

    constexpr uint64_t get_hands_played() const noexcept {
        return hands_played;
    }

    constexpr int64_t get_bankroll_delta() const noexcept {
        return static_cast<int64_t>(ending_bankroll_cents) - static_cast<int64_t>(starting_bankroll_cents);
    }

    constexpr double get_expected_value_per_hand() const noexcept {
        if (hands_played == 0) {
            return 0.0;
        }
        return static_cast<double>(get_bankroll_delta()) / static_cast<double>(hands_played);
    }

    constexpr uint32_t get_starting_bankroll() const noexcept {
        return starting_bankroll_cents;
    }

    constexpr uint32_t get_ending_bankroll() const noexcept {
        return ending_bankroll_cents;
    }

    constexpr void add(const GameStatistics& other) noexcept {
        hands_played += other.hands_played;
        starting_bankroll_cents += other.starting_bankroll_cents;
        ending_bankroll_cents += other.ending_bankroll_cents;
    }
};

#endif
