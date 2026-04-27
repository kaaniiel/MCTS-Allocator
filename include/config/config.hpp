// include/config/config.hpp
#pragma once
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include "toml.hpp"

struct Config
{
    // 1. Default values (Source of truth)
    bool launch = false;
    int numAgents = 3;
    int numObjects = 4;
    int iterations = 100;
    double exploration = 1.414;
    int seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count());
    bool verbose = false;
    double ratioRandom = 1;
    bool saveResults = false;
    bool useSolver = false;

    // Default values for experiments
    int numAgentsMin = 3;
    int numAgentsMax = 3;
    int numObjectsMin = 4;
    int numObjectsMax = 4;
    int seedMin = 42;
    int seedMax = 42;
    double ratioRandomMin = 1.0;
    double ratioRandomMax = 1.0;
    int ratioRandomStep = 1;
    int numberOfTrys = 1;
    double numberOfBudgetStep = 0;
    std::string outputDirectory = "results";

    // 2. Function to generate the default configuration file
    static void generate_default(const std::string &filepath, const Config &default_config)
    {
        toml::table tbl;
        tbl.insert("mcts", toml::table{
                               {"launch", default_config.launch},
                               {"num_agents", default_config.numAgents},
                               {"num_objects", default_config.numObjects},
                               {"iterations", default_config.iterations},
                               {"exploration_constant", default_config.exploration},
                               {"seed", default_config.seed},
                               {"verbose", default_config.verbose},
                               {"ratio_random", default_config.ratioRandom},
                               {"save_results", default_config.saveResults},
                               {"use_solver", default_config.useSolver},
                           });
        tbl.insert("experiments", toml::table{
                                      {"num_agents_min", default_config.numAgentsMin},
                                      {"num_agents_max", default_config.numAgentsMax},
                                      {"num_objects_min", default_config.numObjectsMin},
                                      {"num_objects_max", default_config.numObjectsMax},
                                      {"seed_min", default_config.seedMin},
                                      {"seed_max", default_config.seedMax},
                                      {"ratio_random_min", default_config.ratioRandomMin},
                                      {"ratio_random_max", default_config.ratioRandomMax},
                                      {"ratio_random_step", default_config.ratioRandomStep},
                                      {"number_of_trys", default_config.numberOfTrys},
                                      {"numberOfBudgetStep", default_config.numberOfBudgetStep},
                                      {"verbose", default_config.verbose},
                                      {"output_directory", default_config.outputDirectory},
                                  });

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
    static Config load(const std::string &filepath = "config.toml")
    {
        Config config; // Initialized with default values

        if (!std::filesystem::exists(filepath))
        {
            throw std::runtime_error("[Config] Missing configuration file: " + filepath);
        }

        try
        {
            toml::table tbl = toml::parse_file(filepath);

            config.launch = tbl["mcts"]["launch"].value_or(config.launch);
            config.numAgents = tbl["mcts"]["num_agents"].value_or(config.numAgents);
            config.numObjects = tbl["mcts"]["num_objects"].value_or(config.numObjects);
            config.iterations = tbl["mcts"]["iterations"].value_or(config.iterations);
            config.exploration = tbl["mcts"]["exploration_constant"].value_or(config.exploration);
            config.seed = tbl["mcts"]["seed"].value_or(config.seed);
            config.verbose = tbl["mcts"]["verbose"].value_or(config.verbose);
            config.ratioRandom = tbl["mcts"]["ratio_random"].value_or(config.ratioRandom);
            config.saveResults = tbl["mcts"]["save_results"].value_or(config.saveResults);
            config.useSolver = tbl["mcts"]["use_solver"].value_or(config.useSolver);
            
            // Experiments parameters
            config.numAgentsMin = tbl["experiments"]["num_agents_min"].value_or(config.numAgentsMin);
            config.numAgentsMax = tbl["experiments"]["num_agents_max"].value_or(config.numAgentsMax);
            config.numObjectsMin = tbl["experiments"]["num_objects_min"].value_or(config.numObjectsMin);
            config.numObjectsMax = tbl["experiments"]["num_objects_max"].value_or(config.numObjectsMax);
            config.seedMin = tbl["experiments"]["seed_min"].value_or(config.seedMin);
            config.seedMax = tbl["experiments"]["seed_max"].value_or(config.seedMax);
            config.ratioRandomMin = tbl["experiments"]["ratio_random_min"].value_or(config.ratioRandomMin);
            config.ratioRandomMax = tbl["experiments"]["ratio_random_max"].value_or(config.ratioRandomMax);
            config.ratioRandomStep = tbl["experiments"]["ratio_random_step"].value_or(config.ratioRandomStep);
            config.numberOfTrys = tbl["experiments"]["number_of_trys"].value_or(config.numberOfTrys);
            config.numberOfBudgetStep = tbl["experiments"]["numberOfBudgetStep"].value_or(config.numberOfBudgetStep);
            config.outputDirectory = tbl["experiments"]["output_directory"].value_or(config.outputDirectory);
            
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