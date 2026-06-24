#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include "IAllocator.hpp"
#include "mcts/Preferences.hpp"
#include "mcts/Allocation.hpp"
#include "mcts/Score.hpp"
#include "config/config.hpp"
#include <gurobi_c++.h>

template <typename T>
class Solver : public IAllocator<T>
{
private:
    int seed;
    Preferences<T> prefs;
    std::pair<Allocation, Score> optimalAllocation;
    int timeoutSeconds;
    Config config;
    double executionTimeSeconds = 0.0;

public:
    Solver(const int numAgents, const int numObjects, const int seed = static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count())) : seed(seed), prefs(numAgents, numObjects, false), timeoutSeconds(60) // Default timeout of 1 minute
    {
        prefs.generateRandomPreferences(numAgents * numObjects, seed);
    };

    Solver(const Preferences<T> &preferences) : seed(static_cast<int>(std::chrono::system_clock::now().time_since_epoch().count())), prefs(preferences), timeoutSeconds(60) {};
    Solver(const Preferences<T> &preferences, const int seed) : seed(seed), prefs(preferences), timeoutSeconds(60) {};
    Solver(const Config &config) : seed(), prefs(), timeoutSeconds()
    {
        load_config(config);
    };

public:
    std::pair<Allocation, Score> solve(bool verbose = false)
    {
        return solve(prefs, verbose);
    }

    std::pair<Allocation, Score> solve(const Preferences<T> &inputPrefs, bool verbose = false)
    {
        static std::unique_ptr<GRBEnv> sharedEnv;
        if (!sharedEnv)
        {
            sharedEnv = std::make_unique<GRBEnv>(true);
            sharedEnv->start();
        }

        if (!verbose)
        {
            sharedEnv->set("OutputFlag", "0");
            sharedEnv->set("LogToConsole", "0");
        }
        else
        {
            sharedEnv->set("LogFile", "gurobi.log");
        }

        GRBModel model = GRBModel(*sharedEnv);

        const int numAgents = inputPrefs.getNumAgents();
        const int numObjects = inputPrefs.getNumObjects();

        // Safety: if there are fewer objects than agents, avoid attempting to solve/search.
        if (numObjects < numAgents)
        {
            if (verbose)
            {
                std::cerr << "Solver: number of objects (" << numObjects << ") is less than number of agents (" << numAgents << "). Aborting solve.\n";
            }
            optimalAllocation = std::pair<Allocation, Score>(Allocation(numAgents, numObjects), Score(0.0, verbose));
            return optimalAllocation;
        }

        // Create Boolean Variables
        std::vector<std::vector<GRBVar>> x(numAgents, std::vector<GRBVar>(numObjects));
        for (int i = 0; i < numAgents; ++i)
        {
            for (int j = 0; j < numObjects; ++j)
            {
                x[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + std::to_string(i) + "_" + std::to_string(j));
            }
        }

        // Each object must be assigned to exactly one agent (same semantics as Allocation).
        for (int j = 0; j < numObjects; ++j)
        {
            GRBLinExpr objectConstraint = 0;
            for (int i = 0; i < numAgents; ++i)
            {
                objectConstraint += x[i][j];
            }
            model.addConstr(objectConstraint == 1, "Object_" + std::to_string(j) + "_constraint");
        }

        // Precalculate the log of preferences to avoid computing it multiple times in the objective function
        const double eps = 1e-6;
        std::vector<GRBVar> agentUtility(numAgents);
        std::vector<GRBVar> logAgentUtility(numAgents);
        for (int i = 0; i < numAgents; ++i)
        {
            agentUtility[i] = model.addVar(eps, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "u_" + std::to_string(i));
            logAgentUtility[i] = model.addVar(-GRB_INFINITY, GRB_INFINITY, 0.0, GRB_CONTINUOUS, "log_u_" + std::to_string(i));

            GRBLinExpr utilExpr = eps;
            for (int j = 0; j < numObjects; ++j)
            {
                utilExpr += static_cast<double>(inputPrefs.getPreference(i, j)) * x[i][j];
            }
            model.addConstr(agentUtility[i] == utilExpr, "utility_def_" + std::to_string(i));
            model.addGenConstrLog(agentUtility[i], logAgentUtility[i], "log_link_" + std::to_string(i));
        }

        // Define the objective function (e.g., maximize total utility)
        GRBLinExpr objective = 0;
        for (int i = 0; i < numAgents; ++i)
        {
            objective += logAgentUtility[i];
        }
        model.setObjective(objective, GRB_MAXIMIZE);

        model.set(GRB_DoubleParam_TimeLimit, timeoutSeconds);
        // Optimize the model
        model.optimize();

        this->executionTimeSeconds = model.get(GRB_DoubleAttr_Runtime);

        Allocation allocation(numAgents, numObjects);
        double score = 0.0;

        // Output the results
        const int status = model.get(GRB_IntAttr_Status);
        if (status != GRB_OPTIMAL)
        {
            if (verbose)
            {
                std::cout << "No optimal solution found. Gurobi status = " << status << std::endl;
            }
            optimalAllocation = std::pair<Allocation, Score>(Allocation(numAgents, numObjects), Score(0.0, verbose));
            return optimalAllocation; // Return an empty allocation and zero score if no solution is found
        }

        // Print the optimal assignment only in verbose mode
        if (verbose)
        {
            std::cout << "Optimal Assignment:" << std::endl;
        }
        std::vector<int> allocVec(numObjects, -1);
        for (int i = 0; i < numAgents; ++i)
        {
            for (int j = 0; j < numObjects; ++j)
            {
                if (x[i][j].get(GRB_DoubleAttr_X) > 0.5) // Check if the variable is set to 1
                {
                    const double pref = static_cast<double>(inputPrefs.getPreference(i, j));
                    allocVec[j] = i;
                    if (verbose)
                    {
                        std::cout << "Agent " << i << " assigned to Object " << j << " with preference " << pref << std::endl;
                    }
                }
            }
        }

        Allocation alloc(numAgents, allocVec);

        std::pair<Allocation, Score> result = std::pair<Allocation, Score>(alloc, Score(model.get(GRB_DoubleAttr_ObjVal), verbose));
        optimalAllocation = result;

        return result; // Return the result by value; internal storage is released
    }

    const Preferences<T> &getPreferences() const { return prefs; }

    void clear()
    {
        // Reset stored optimal allocation and score
        optimalAllocation.first.clear();
        optimalAllocation.second = Score();

        // Release preferences memory
        prefs.clear();
    }

    void load_config(const Config &config)
    {
        seed = config.seed;
        prefs = Preferences<T>(config.numAgents, config.numObjects, config.verbose);
        prefs.generateRandomPreferences(config.numAgents * config.numObjects, config.seed);
        timeoutSeconds = config.solverTimeoutSeconds;
        this->config = config;
    }

    std::string to_json(const bool add_metrics = false)
    {
        int numAgents = optimalAllocation.first.getNumAgents();
        int numObjects = optimalAllocation.first.getNumObjects();

        std::ostringstream oss;

        oss << "{\n";
        oss << "  \"num_agents\": " << numAgents << ",\n";
        oss << "  \"num_objects\": " << numObjects << ",\n";
        // add preferences matrix
        oss << "  \"preferences\": [\n";
        oss << "  \"execution_time_seconds\": " << executionTimeSeconds << ",\n"; // <-- Ajoutez cette ligne
        for (int i = 0; i < numAgents; ++i)
        {
            oss << "    [";
            for (int j = 0; j < numObjects; ++j)
            {
                oss << prefs.getPreference(i, j);
                if (j < numObjects - 1)
                    oss << ", ";
            }
            oss << "]";
            if (i < numAgents - 1)
                oss << ",\n";
        }

        oss << "  ],\n";
        // add best allocation and score
        const Allocation &bestAlloc = optimalAllocation.first;
        const Score &bestScore = optimalAllocation.second;
        oss << "  \"best_allocation\": [";
        const std::vector<int> &allocVec = bestAlloc.getAllocation();
        for (size_t i = 0; i < allocVec.size(); ++i)
        {
            oss << allocVec[i];
            if (i < allocVec.size() - 1)
                oss << ", ";
        }
        oss << "],\n";
        oss << "  \"best_score\": " << bestScore.getScore() << ",\n";
        oss << "  \"metrics\": {\n";
        if (add_metrics)
        {
            for (const auto &[name, metricFunc] : getMetricsRegistry<T>())
            {
                double metricValue = metricFunc(prefs, bestAlloc);
                oss << "    \"" << name << "\": " << metricValue;
                if (name != getMetricsRegistry<T>().rbegin()->first)
                    oss << ",";
                oss << "\n";
            }
        }
        oss << "  }\n";
        oss << "}\n";
        return oss.str();
    }

    void save_results_json(const std::string &filename, const bool add_metrics = false)
    {
        std::string results_dir = "results";
        if (!std::filesystem::exists(results_dir))
        {
            std::filesystem::create_directory(results_dir);
        }

        std::ofstream file(results_dir + "/" + filename);
        if (file.is_open())
        {
            // On appelle notre nouvelle fonction to_json() !
            file << to_json(add_metrics);

            // Ne pas l'afficher dans le terminal si l'option -J est activée
            if (!config.terminalJSONOutput)
            {
                std::cout << "[Results] Saved results to: " << results_dir + "/" + filename << "\n";
            }
        }
        else
        {
            std::cerr << "[Results] Error: unable to save results to " << results_dir + "/" + filename << "\n";
        }
    }
};
#endif // SOLVER_HPP