#ifndef BLACKJACK_GAME_RULESET_HPP
#define BLACKJACK_GAME_RULESET_HPP

#include <cstdint>

namespace blackjack::game {
    class Ruleset {
    public:
        uint8_t blackjack_payout_num;
        uint8_t blackjack_payout_denom;
        bool dealer_hits_soft_17;
        bool surrender_allowed;
        bool double_after_split_allowed;
        uint8_t max_split_hands;

        static constexpr Ruleset default_vegas() noexcept {
            return Ruleset(3, 2, false, true, true, 4);
        }

        static constexpr Ruleset tight_h17_no_surrender() noexcept {
            return Ruleset(6, 5, true, false, false, 2);
        }

        [[nodiscard]] constexpr uint32_t calculate_blackjack_payout(uint32_t bet) const noexcept {
            return bet + (bet * this->blackjack_payout_num / this->blackjack_payout_denom);
        }

    private:
        constexpr Ruleset(
            uint8_t bj_num,
            uint8_t bj_denom,
            bool dealer_hits_soft_17_,
            bool surrender_allowed_,
            bool double_after_split_allowed_,
            uint8_t max_split_hands_
        ) noexcept
            : blackjack_payout_num(bj_num)
            , blackjack_payout_denom(bj_denom)
            , dealer_hits_soft_17(dealer_hits_soft_17_)
            , surrender_allowed(surrender_allowed_)
            , double_after_split_allowed(double_after_split_allowed_)
            , max_split_hands(max_split_hands_)
        {}
    };
}

#endif
