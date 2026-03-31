// include/config/config.hpp
#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#include "toml.hpp"

struct MCTSConfig
{
    // 1. Default values (Source of truth)
    int numAgents = 3;
    int numObjects = 4;
    bool parallelRun = false;
    int iterations = 100;
    double exploration = 1.414;
    int threads = -1;
    int seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    bool verbose = false;

    // 2. Function to generate the default configuration file
    static void generate_default(const std::string &filepath, const MCTSConfig &default_config)
    {
        // Build the TOML structure
        toml::table tbl;
        tbl.insert("mcts", toml::table{
                               {"num_agents", default_config.numAgents},
                               {"num_objects", default_config.numObjects},
                               {"parallel_run", default_config.parallelRun},
                               {"iterations", default_config.iterations},
                               {"exploration_constant", default_config.exploration},
                               {"num_threads", default_config.threads},
                               {"seed", default_config.seed},
                               {"verbose", default_config.verbose},
                           });

        // Write to file
        std::ofstream file(filepath);
        if (file.is_open())
        {
            file << "# Automatically generated configuration file\n";
            file << tbl << "\n";
            std::cout << "[Config] Default file created: " << filepath << "\n";
        }
        else
        {
            std::cerr << "[Config] Error: unable to create " << filepath << "\n";
        }
    }

    // 3. Main load function
    static MCTSConfig load(const std::string &filepath = "config.toml")
    {
        MCTSConfig config; // Initialized with default values

        // Check if the file exists
        if (!std::filesystem::exists(filepath))
        {
            std::cout << "[Config] File not found. Generating default...\n";
            generate_default(filepath, config);
            return config; // Return the default config
        }

        // If the file exists, parse it
        try
        {
            toml::table tbl = toml::parse_file(filepath);

            // Safe extraction:
            // If the key is missing OR the type is wrong (e.g., string instead of int),
            // value_or() will silently fail and keep the default value from 'config'.
            config.numAgents = tbl["mcts"]["num_agents"].value_or(config.numAgents);
            config.numObjects = tbl["mcts"]["num_objects"].value_or(config.numObjects);
            config.iterations = tbl["mcts"]["iterations"].value_or(config.iterations);
            config.exploration = tbl["mcts"]["exploration_constant"].value_or(config.exploration);
            config.threads = tbl["mcts"]["num_threads"].value_or(config.threads);
            config.seed = tbl["mcts"]["seed"].value_or(config.seed);
            config.verbose = tbl["mcts"]["verbose"].value_or(config.verbose);
            std::cout << "[Config] Configuration loaded from " << filepath << "\n";
        }
        catch (const toml::parse_error &err)
        {
            // Handle malformed TOML files (e.g., missing brackets)
            std::cerr << "[Config] Critical syntax error in TOML:\n"
                      << err << "\n";
            std::cerr << "[Config] Falling back to default parameters.\n";
        }

        return config;
    }
};