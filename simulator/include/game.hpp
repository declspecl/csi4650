#ifndef GAME_HPP
#define GAME_HPP

#include "player.hpp"
#include "shoe.hpp"

#include <array>
#include <cstdint>

constexpr uint8_t MAX_NON_DEALER_PLAYERS = 7;

class Game {
private:
    std::array<Player, MAX_NON_DEALER_PLAYERS> players;
    Shoe shoe;
    Player dealer;
    uint8_t player_count;

public:
    constexpr Game() noexcept;
};

constexpr Game::Game() noexcept
    : players({})
    , shoe(Shoe())
    , dealer(Player())
    , player_count(MAX_NON_DEALER_PLAYERS)
{}

#endif