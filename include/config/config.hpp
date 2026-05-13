// include/config/config.hpp
#pragma once
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <vector>

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
    std::vector<int> stepAgents = {1};
    int numObjectsMin = 4;
    int numObjectsMax = 4;
    std::vector<int> stepObjects = {1};
    double budgetMultiplier = 1.0;
    int seedMin = 42;
    int seedMax = 42;
    std::vector<int> stepSeeds = {1};
    double ratioRandomMin = 1.0;
    double ratioRandomMax = 1.0;
    double ratioRandomStep = 1.0;
    int numberOfTrys = 1;
    double numberOfBudgetStep = 0;
    bool enableMetrics = false;
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
        {
            toml::table experiments_tbl;
            experiments_tbl.insert("num_agents_min", default_config.numAgentsMin);
            experiments_tbl.insert("num_agents_max", default_config.numAgentsMax);
            // convert stepAgents to toml::array
            {
                toml::array arr;
                for (int v : default_config.stepAgents)
                    arr.push_back(v);
                experiments_tbl.insert("step_agents", std::move(arr));
            }
            experiments_tbl.insert("num_objects_min", default_config.numObjectsMin);
            experiments_tbl.insert("num_objects_max", default_config.numObjectsMax);
            // convert stepObjects to toml::array
            {
                toml::array arr;
                for (int v : default_config.stepObjects)
                    arr.push_back(v);
                experiments_tbl.insert("step_objects", std::move(arr));
            }
            experiments_tbl.insert("seed_min", default_config.seedMin);
            experiments_tbl.insert("seed_max", default_config.seedMax);
            // convert stepSeeds to toml::array
            {
                toml::array arr;
                for (int v : default_config.stepSeeds)
                    arr.push_back(v);
                experiments_tbl.insert("step_seeds", std::move(arr));
            }
            experiments_tbl.insert("ratio_random_min", default_config.ratioRandomMin);
            experiments_tbl.insert("ratio_random_max", default_config.ratioRandomMax);
            experiments_tbl.insert("ratio_random_step", default_config.ratioRandomStep);
            experiments_tbl.insert("number_of_trys", default_config.numberOfTrys);
            experiments_tbl.insert("number_of_budget_step", default_config.numberOfBudgetStep);
            experiments_tbl.insert("budget_multiplier", default_config.budgetMultiplier);
            experiments_tbl.insert("verbose", default_config.verbose);
            experiments_tbl.insert("enable_metrics", default_config.enableMetrics);
            experiments_tbl.insert("output_directory", default_config.outputDirectory);
            tbl.insert("experiments", std::move(experiments_tbl));
        }

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
            config.numAgents = static_cast<int>(tbl["mcts"]["num_agents"].value_or(static_cast<int64_t>(config.numAgents)));
            config.numObjects = static_cast<int>(tbl["mcts"]["num_objects"].value_or(static_cast<int64_t>(config.numObjects)));
            config.iterations = static_cast<int>(tbl["mcts"]["iterations"].value_or(static_cast<int64_t>(config.iterations)));
            config.exploration = tbl["mcts"]["exploration_constant"].value_or(config.exploration);
            config.seed = static_cast<int>(tbl["mcts"]["seed"].value_or(static_cast<int64_t>(config.seed)));
            config.verbose = tbl["mcts"]["verbose"].value_or(config.verbose);
            config.ratioRandom = tbl["mcts"]["ratio_random"].value_or(config.ratioRandom);
            config.saveResults = tbl["mcts"]["save_results"].value_or(config.saveResults);
            config.useSolver = tbl["mcts"]["use_solver"].value_or(config.useSolver);

            // Experiments parameters
            config.numAgentsMin = static_cast<int>(tbl["experiments"]["num_agents_min"].value_or(static_cast<int64_t>(config.numAgentsMin)));
            config.numAgentsMax = static_cast<int>(tbl["experiments"]["num_agents_max"].value_or(static_cast<int64_t>(config.numAgentsMax)));

            if (auto arr = tbl["experiments"]["step_agents"].as_array())
            {
                config.stepAgents.clear();
                for (const auto &elem : *arr)
                {
                    config.stepAgents.push_back(static_cast<int>(elem.value_or(static_cast<int64_t>(0))));
                }
            }

            config.numObjectsMin = static_cast<int>(tbl["experiments"]["num_objects_min"].value_or(static_cast<int64_t>(config.numObjectsMin)));
            config.numObjectsMax = static_cast<int>(tbl["experiments"]["num_objects_max"].value_or(static_cast<int64_t>(config.numObjectsMax)));

            if (auto arr = tbl["experiments"]["step_objects"].as_array())
            {
                config.stepObjects.clear();
                for (const auto &elem : *arr)
                {
                    config.stepObjects.push_back(static_cast<int>(elem.value_or(static_cast<int64_t>(0))));
                }
            }

            config.seedMin = static_cast<int>(tbl["experiments"]["seed_min"].value_or(static_cast<int64_t>(config.seedMin)));
            config.seedMax = static_cast<int>(tbl["experiments"]["seed_max"].value_or(static_cast<int64_t>(config.seedMax)));

            if (auto arr = tbl["experiments"]["step_seeds"].as_array())
            {
                config.stepSeeds.clear();
                for (const auto &elem : *arr)
                {
                    config.stepSeeds.push_back(static_cast<int>(elem.value_or(static_cast<int64_t>(0))));
                }
            }

            config.ratioRandomMin = tbl["experiments"]["ratio_random_min"].value_or(config.ratioRandomMin);
            config.ratioRandomMax = tbl["experiments"]["ratio_random_max"].value_or(config.ratioRandomMax);
            config.ratioRandomStep = tbl["experiments"]["ratio_random_step"].value_or(config.ratioRandomStep);
            config.numberOfTrys = static_cast<int>(tbl["experiments"]["number_of_trys"].value_or(static_cast<int64_t>(config.numberOfTrys)));
            config.numberOfBudgetStep = tbl["experiments"]["number_of_budget_step"].value_or(config.numberOfBudgetStep);
            config.budgetMultiplier = tbl["experiments"]["budget_multiplier"].value_or(config.budgetMultiplier);
            config.enableMetrics = tbl["experiments"]["enable_metrics"].value_or(config.enableMetrics);
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