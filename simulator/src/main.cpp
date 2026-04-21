#include <blackjack/sim/run_config.hpp>
#include <blackjack/sim/run_report.hpp>
#include <blackjack/sim/simulation.hpp>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>

using blackjack::sim::OutputFormat;
using blackjack::sim::SimRunConfig;
using blackjack::sim::print_usage;
using blackjack::sim::run_simulation;
using blackjack::sim::try_parse_run_config;
using blackjack::sim::write_json_benchmark_report;
using blackjack::sim::write_json_report;
using blackjack::sim::write_text_report;

int main(int argc, char* argv[]) {
    std::string err;
    std::optional<SimRunConfig> cfg_opt = try_parse_run_config(argc, argv, err);
    if (!cfg_opt.has_value()) {
        if (err == "help") {
            print_usage(argv[0]);
            return 0;
        }
        std::cerr << err << "\n";
        print_usage(argv[0]);
        return 1;
    }

    SimRunConfig cfg = *cfg_opt;

    using clock = std::chrono::steady_clock;
    using ms    = std::chrono::duration<double, std::milli>;

    if (cfg.benchmark) {
        SimRunConfig serial_cfg = cfg;
        serial_cfg.threads      = 1;

        auto   t0        = clock::now();
        auto   serial_s  = run_simulation(serial_cfg);
        double serial_ms = ms(clock::now() - t0).count();

        auto   t1          = clock::now();
        auto   parallel_s  = run_simulation(cfg);
        double parallel_ms = ms(clock::now() - t1).count();

        if (cfg.format == OutputFormat::JSON) {
            write_json_benchmark_report(std::cout, cfg, serial_s, parallel_s);
            std::clog << "observed serial_wall_time_ms=" << std::setprecision(17) << serial_ms
                      << " parallel_wall_time_ms=" << parallel_ms
                      << " speedup=" << ((parallel_ms > 0.0) ? (serial_ms / parallel_ms) : 0.0)
                      << "\n";
        } else {
            std::cout << "=== Serial (1 thread) ===\n";
            write_text_report(std::cout, serial_cfg, serial_s, serial_ms);
            std::cout << "\n=== Parallel (" << cfg.threads << " threads) ===\n";
            write_text_report(std::cout, cfg, parallel_s, parallel_ms);
            double speedup = (parallel_ms > 0.0) ? (serial_ms / parallel_ms) : 0.0;
            if (!std::isfinite(speedup)) {
                speedup = 0.0;
            }
            std::cout << "\n[observed]\n";
            std::cout << "speedup:             " << speedup << "x\n";
        }
        return 0;
    }

    auto   t0      = clock::now();
    auto   stats   = run_simulation(cfg);
    double wall_ms = ms(clock::now() - t0).count();

    if (cfg.format == OutputFormat::JSON) {
        write_json_report(std::cout, cfg, stats);
        std::clog << "observed wall_time_ms=" << std::setprecision(17) << wall_ms << "\n";
    } else {
        write_text_report(std::cout, cfg, stats, wall_ms);
    }

    return 0;
}
