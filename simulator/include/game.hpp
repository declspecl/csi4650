#ifndef GAME_HPP
#define GAME_HPP

#include "player.hpp"
#include "shoe.hpp"
#include "betting_config.hpp"
#include "game_statistics.hpp"
#include "hand_state.hpp"
#include "hand_outcome.hpp"

#include <array>
#include <cstdint>

constexpr uint8_t MAX_NON_DEALER_PLAYERS = 7;

class Game {
private:
    std::array<Player, MAX_NON_DEALER_PLAYERS> players;
    Shoe shoe;
    Player dealer;
    uint8_t player_count;
    BettingConfig betting_config;
    uint64_t hands_played_count;
    uint32_t starting_bankroll_total;

public:
    constexpr Game() noexcept;
    explicit constexpr Game(const BettingConfig& config) noexcept;

    constexpr void initialize_round() noexcept;
    constexpr void resolve_hand(Player& player, uint8_t hand_index) noexcept;
    constexpr void resolve_all_hands() noexcept;

    constexpr HandOutcome determine_outcome(const Hand& player_hand, const Hand& dealer_hand) const noexcept;
    constexpr uint32_t calculate_payout(HandOutcome outcome, uint32_t bet) const noexcept;


    constexpr const BettingConfig& get_betting_config() const noexcept;
    constexpr Player& get_player(uint8_t index) noexcept;
    constexpr const Player& get_dealer() const noexcept;

    constexpr GameStatistics aggregate_statistics() const noexcept;
    constexpr void finalize_player_statistics() noexcept;
};

constexpr Game::Game() noexcept
    : players({})
    , shoe(Shoe())
    , dealer(Player())
    , player_count(MAX_NON_DEALER_PLAYERS)
    , betting_config(BettingConfig())
    , hands_played_count(0)
    , starting_bankroll_total(0)
{}

constexpr Game::Game(const BettingConfig& config) noexcept
    : players({})
    , shoe(Shoe())
    , dealer(Player())
    , player_count(MAX_NON_DEALER_PLAYERS)
    , betting_config(config)
    , hands_played_count(0)
    , starting_bankroll_total(0)
{}

constexpr void Game::initialize_round() noexcept {
    hands_played_count = 0;
    starting_bankroll_total = 0;

    for (uint8_t i = 0; i < player_count; i++) {
        players[i].initialize_bankroll(betting_config.get_initial_bankroll());
        starting_bankroll_total += betting_config.get_initial_bankroll();
    }

    dealer.clear_hand(0);
}

constexpr HandOutcome Game::determine_outcome(const Hand& player_hand, const Hand& dealer_hand) const noexcept {
    bool was_split = (player_hand.get_origin() == HandOrigin::SPLIT);

    if (player_hand.is_bust()) {
        return HandOutcome::PLAYER_BUST_LOSS;
    }
    else if (player_hand.is_blackjack() && !was_split) {
        if (dealer_hand.is_blackjack()) {
            return HandOutcome::PUSH;
        } else {
            return HandOutcome::BLACKJACK_WIN;
        }
    }
    else if (dealer_hand.is_bust()) {
        return HandOutcome::DEALER_BUST_WIN;
    }
    else if (dealer_hand.is_blackjack()) {
        return HandOutcome::DEALER_BLACKJACK_LOSS;
    }
    else {
        uint8_t player_value = player_hand.get_value();
        uint8_t dealer_value = dealer_hand.get_value();

        if (player_value > dealer_value) {
            return HandOutcome::REGULAR_WIN;
        } else if (player_value < dealer_value) {
            return HandOutcome::DEALER_WIN_LOSS;
        } else {
            return HandOutcome::PUSH;
        }
    }
}

constexpr void Game::resolve_hand(Player& player, uint8_t hand_index) noexcept {
    const Hand& player_hand = player.get_hand(hand_index);
    const Hand& dealer_hand = dealer.get_hand(0);

    HandOutcome outcome = determine_outcome(player_hand, dealer_hand);

    uint32_t payout = calculate_payout(outcome, player_hand.get_bet());
    if (payout > 0) {
        player.add_to_bankroll(payout);
    }

    hands_played_count++;
}

constexpr uint32_t Game::calculate_payout(HandOutcome outcome, uint32_t bet) const noexcept {
    switch (outcome) {
        case HandOutcome::BLACKJACK_WIN:
            return betting_config.calculate_blackjack_payout(bet);

        case HandOutcome::REGULAR_WIN:
        case HandOutcome::DEALER_BUST_WIN:
            return bet + bet;

        case HandOutcome::PUSH:
            return bet;

        case HandOutcome::PLAYER_BUST_LOSS:
        case HandOutcome::DEALER_WIN_LOSS:
        case HandOutcome::DEALER_BLACKJACK_LOSS:
            return 0;
    }
}

constexpr void Game::resolve_all_hands() noexcept {
    for (uint8_t p = 0; p < player_count; p++) {
        Player& player = players[p];
        for (uint8_t h = 0; h < player.get_active_hand_count(); h++) {
            resolve_hand(player, h);
        }
    }
}

constexpr const BettingConfig& Game::get_betting_config() const noexcept {
    return betting_config;
}

constexpr Player& Game::get_player(uint8_t index) noexcept {
    assert(index < player_count);
    return players[index];
}

constexpr const Player& Game::get_dealer() const noexcept {
    return dealer;
}

constexpr void Game::finalize_player_statistics() noexcept {
}

constexpr GameStatistics Game::aggregate_statistics() const noexcept {
    uint64_t total_ending = 0;
    for (uint8_t i = 0; i < player_count; i++) {
        total_ending += players[i].get_bankroll();
    }
    return GameStatistics(hands_played_count, starting_bankroll_total, static_cast<uint32_t>(total_ending));
}

#endif
