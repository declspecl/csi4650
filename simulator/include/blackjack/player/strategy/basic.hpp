#ifndef BLACKJACK_PLAYER_STRATEGY_BASIC_HPP
#define BLACKJACK_PLAYER_STRATEGY_BASIC_HPP

#include <blackjack/card/card.hpp>
#include <blackjack/card/rank.hpp>
#include <blackjack/hand/hand.hpp>
#include <blackjack/player/strategy/strategy.hpp>

#include <cstdint>

namespace blackjack::player::strategy {
    /**
     * Textbook multi-deck basic strategy (S17, late surrender, DAS configurable).
     */
    class BasicStrategy : public PlayerStrategy {
    private:
        bool das_allowed;

    public:
        constexpr BasicStrategy() noexcept : das_allowed(true) {}
        constexpr explicit BasicStrategy(bool double_after_split_allowed) noexcept
            : das_allowed(double_after_split_allowed) {}

        [[nodiscard]] Decision get_decision(const GameContext& context) const noexcept override;

    private:
        [[nodiscard]] static bool should_split(card::Rank pair_rank, uint8_t upcard_value, bool das) noexcept;
        [[nodiscard]] static Decision soft_decision(uint8_t hand_value, uint8_t upcard_value, bool can_double) noexcept;
        [[nodiscard]] static Decision hard_decision(uint8_t hand_value, uint8_t upcard_value, bool can_double, bool can_surrender) noexcept;
    };

    inline Decision BasicStrategy::get_decision(const GameContext& context) const noexcept {
        const Hand& hand = context.get_own_hand();
        uint8_t up = context.get_dealer_upcard().get_max_value();

        if (context.can_split() && hand.card_count() == 2) {
            const Card* cards = hand.get_cards_data();
            card::Rank pair_rank = cards[0].get_rank();
            bool is_pair = (pair_rank == cards[1].get_rank());
            if (is_pair && should_split(pair_rank, up, this->das_allowed)) {
                return Decision::SPLIT;
            }
        }

        uint8_t value = hand.get_value();
        if (hand.is_soft()) {
            return soft_decision(value, up, context.can_double());
        }
        return hard_decision(value, up, context.can_double(), context.can_surrender());
    }

    inline bool BasicStrategy::should_split(card::Rank pair_rank, uint8_t up, bool das) noexcept {
        switch (pair_rank) {
            case card::Rank::ACE:
            case card::Rank::EIGHT:
                return true;

            case card::Rank::NINE:
                return !(up == 7 || up >= 10);

            case card::Rank::SEVEN:
                return up >= 2 && up <= 7;

            case card::Rank::SIX:
                return das ? (up >= 2 && up <= 6) : (up >= 3 && up <= 6);

            case card::Rank::FOUR:
                return das && (up == 5 || up == 6);

            case card::Rank::THREE:
            case card::Rank::TWO:
                return das ? (up >= 2 && up <= 7) : (up >= 4 && up <= 7);

            case card::Rank::FIVE:
            case card::Rank::TEN:
            case card::Rank::JACK:
            case card::Rank::QUEEN:
            case card::Rank::KING:
                return false;
        }
        return false;
    }

    inline Decision BasicStrategy::soft_decision(uint8_t value, uint8_t up, bool can_double) noexcept {
        switch (value) {
            case 20:
            case 21:
                return Decision::STAND;

            case 19:
                return Decision::STAND;

            case 18:
                if (up >= 3 && up <= 6) {
                    return can_double ? Decision::DOUBLE : Decision::STAND;
                }
                if (up == 2 || up == 7 || up == 8) {
                    return Decision::STAND;
                }
                return Decision::HIT;

            case 17:
                if (up >= 3 && up <= 6) {
                    return can_double ? Decision::DOUBLE : Decision::HIT;
                }
                return Decision::HIT;

            case 16:
            case 15:
                if (up >= 4 && up <= 6) {
                    return can_double ? Decision::DOUBLE : Decision::HIT;
                }
                return Decision::HIT;

            case 14:
            case 13:
                if (up == 5 || up == 6) {
                    return can_double ? Decision::DOUBLE : Decision::HIT;
                }
                return Decision::HIT;

            default:
                return Decision::HIT;
        }
    }

    inline Decision BasicStrategy::hard_decision(uint8_t value, uint8_t up, bool can_double, bool can_surrender) noexcept {
        if (value >= 17) {
            return Decision::STAND;
        }

        if (value == 16) {
            if (up >= 2 && up <= 6) {
                return Decision::STAND;
            }
            if (can_surrender && (up == 9 || up == 10 || up == 11)) {
                return Decision::SURRENDER;
            }
            return Decision::HIT;
        }

        if (value == 15) {
            if (up >= 2 && up <= 6) {
                return Decision::STAND;
            }
            if (can_surrender && up == 10) {
                return Decision::SURRENDER;
            }
            return Decision::HIT;
        }

        if (value == 14 || value == 13) {
            return (up >= 2 && up <= 6) ? Decision::STAND : Decision::HIT;
        }

        if (value == 12) {
            return (up >= 4 && up <= 6) ? Decision::STAND : Decision::HIT;
        }

        if (value == 11) {
            return can_double ? Decision::DOUBLE : Decision::HIT;
        }

        if (value == 10) {
            if (up >= 2 && up <= 9) {
                return can_double ? Decision::DOUBLE : Decision::HIT;
            }
            return Decision::HIT;
        }

        if (value == 9) {
            if (up >= 3 && up <= 6) {
                return can_double ? Decision::DOUBLE : Decision::HIT;
            }
            return Decision::HIT;
        }

        return Decision::HIT;
    }
}

#endif
