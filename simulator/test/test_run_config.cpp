#include <blackjack/sim/run_config.hpp>
#include <blackjack/sim/run_report.hpp>
#include <blackjack/sim/simulation.hpp>

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <string_view>

using blackjack::sim::JSON_SCHEMA_VERSION;
using blackjack::sim::OutputFormat;
using blackjack::sim::SimRunConfig;
using blackjack::sim::StrategyKind;
using blackjack::sim::round_seed;
using blackjack::sim::run_simulation;
using blackjack::sim::try_parse_run_config;
using blackjack::sim::write_json_report;

namespace {

    [[nodiscard]] std::vector<char*>
    make_argv(const std::vector<std::string>& args) {
        std::vector<char*> out;
        out.reserve(args.size());
        for (const auto& s : args) {
            out.push_back(const_cast<char*>(s.c_str()));
        }
        return out;
    }
}

TEST(RunConfigParse, HelpReturnsNulloptWithHelpError) {
    std::vector<std::string> args = {"prog", "--help"};
    auto                         argv = make_argv(args);
    std::string                  err;
    auto                         cfg  = try_parse_run_config(
        static_cast<int>(argv.size()),
        argv.data(),
        err
    );
    EXPECT_FALSE(cfg.has_value());
    EXPECT_EQ(err, "help");
}

TEST(RunConfigParse, UnknownArgumentFails) {
    std::vector<std::string> args = {"prog", "--not-a-flag"};
    auto                     argv = make_argv(args);
    std::string              err;
    auto                     cfg  = try_parse_run_config(
        static_cast<int>(argv.size()),
        argv.data(),
        err
    );
    EXPECT_FALSE(cfg.has_value());
    EXPECT_FALSE(err.empty());
}

TEST(RunConfigParse, GamesMustBePositive) {
    std::vector<std::string> args = {"prog", "--games", "0"};
    auto                     argv = make_argv(args);
    std::string              err;
    auto                     cfg  = try_parse_run_config(
        static_cast<int>(argv.size()),
        argv.data(),
        err
    );
    EXPECT_FALSE(cfg.has_value());
}

TEST(RunConfigParse, SeedAndFormatJson) {
    std::vector<std::string> args = {
        "prog",
        "--seed",
        "42",
        "--games",
        "10",
        "--rounds",
        "5",
        "--threads",
        "2",
        "--strategy",
        "basic",
        "--format",
        "json",
    };
    auto        argv = make_argv(args);
    std::string err;
    auto        cfg  = try_parse_run_config(
        static_cast<int>(argv.size()),
        argv.data(),
        err
    );
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->base_seed, 42u);
    EXPECT_EQ(cfg->game_count, 10u);
    EXPECT_EQ(cfg->rounds_per_game, 5u);
    EXPECT_EQ(cfg->threads, 2);
    EXPECT_EQ(cfg->strategy, StrategyKind::BASIC);
    EXPECT_EQ(cfg->format, OutputFormat::JSON);
}

TEST(RunConfigParse, ExtendedStrategiesAreAccepted) {
    std::vector<std::string> names = {
        "always-stand",
        "surrender-first",
        "double-first",
        "hi-lo",
    };

    for (const auto& name : names) {
        std::vector<std::string> args = {"prog", "--strategy", name};
        auto                     argv = make_argv(args);
        std::string              err;
        auto                     cfg  = try_parse_run_config(
            static_cast<int>(argv.size()),
            argv.data(),
            err
        );
        ASSERT_TRUE(cfg.has_value()) << name;
        EXPECT_TRUE(err.empty()) << name;
    }
}

TEST(RunConfigSeedScheme, MatchesDocumentedFormula) {
    SimRunConfig cfg{};
    cfg.base_seed       = 100;
    cfg.rounds_per_game = 7;
    EXPECT_EQ(round_seed(cfg, 0, 0), 100u);
    EXPECT_EQ(round_seed(cfg, 1, 0), 107u);
    EXPECT_EQ(round_seed(cfg, 1, 3), 110u);
}

TEST(SimulationReproducibility, IdenticalRunsMatchStatistics) {
    SimRunConfig cfg{};
    cfg.base_seed       = 0xC0FFEEu;
    cfg.game_count      = 8;
    cfg.rounds_per_game = 12;
    cfg.threads         = 4;
    cfg.strategy        = StrategyKind::BASIC;

    auto a = run_simulation(cfg);
    auto b = run_simulation(cfg);

    EXPECT_EQ(a.get_hands_played(), b.get_hands_played());
    EXPECT_EQ(a.get_starting_bankroll(), b.get_starting_bankroll());
    EXPECT_EQ(a.get_ending_bankroll(), b.get_ending_bankroll());
    EXPECT_EQ(a.get_bankroll_delta(), b.get_bankroll_delta());
    EXPECT_EQ(a.get_expected_value_per_hand(), b.get_expected_value_per_hand());
}

TEST(JsonExportShape, HasExpectedTopLevelKeys) {
    SimRunConfig                         cfg{};
    cfg.threads                          = 1;
    blackjack::game::GameStatistics st(10, 1000, 1100);
    std::ostringstream                 ss;
    write_json_report(ss, cfg, st);
    const std::string j = ss.str();

    EXPECT_NE(j.find(std::string("\"schema_version\":") + std::to_string(JSON_SCHEMA_VERSION)), std::string::npos);
    EXPECT_NE(j.find("\"run\""), std::string::npos);
    EXPECT_NE(j.find("\"results\""), std::string::npos);
    EXPECT_NE(j.find("\"seed_scheme\":\"base_plus_game_times_rounds_plus_round\""), std::string::npos);
}
