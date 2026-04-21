#ifndef BLACKJACK_SIM_SIMULATION_HPP
#define BLACKJACK_SIM_SIMULATION_HPP

#include <blackjack/game/game_statistics.hpp>
#include <blackjack/sim/run_config.hpp>

namespace blackjack::sim {

    [[nodiscard]] game::GameStatistics run_simulation(const SimRunConfig& cfg);
}

#endif
