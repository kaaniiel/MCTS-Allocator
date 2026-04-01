#include <iostream>
#include <numeric>
#include <iostream>
#include <clocale>

#include "config/config.hpp"
#include "config/CLI11.hpp" // Assure-toi que le chemin d'inclusion est correct
#include "metrics/Utility.hpp"
#include "mcts/Preferences.hpp"
#include "mcts/Allocation.hpp"
#include "mcts/Node.hpp"
#include "mcts/MCTS.hpp"
#include "mcts_allocation_graph.hpp"
#include "omp.h"
int testPreferences()
{
    int numAgents = 3;
    int numObjects = 4;
    // int seed = 42;
    int totalPerAgents = 10;
    std::cout << "Generating random preferences for " << numAgents << " agents and " << numObjects << " objects with a total preference score of " << totalPerAgents << " for each agent." << std::endl;
    Preferences<int> prefs = Preferences<int>(numAgents, numObjects); // Create preferences for 3 agents and 4 objects
    // Print the generated preference
    prefs.generateRandomPreferences(totalPerAgents); // Generate random preferences for the agents and objects
    std::cout << "Generated Preferences:" << std::endl;
    for (int agent = 0; agent < prefs.getNumAgents(); agent++)
    {
        std::cout << "Agent " << agent << " preferences: ";
        for (int object = 0; object < prefs.getNumObjects(); ++object)
        {
            std::cout << prefs.getPreference(agent, object) << " ";
        }
        std::vector<int> agentPrefs = prefs.getPreference(agent);
        std::cout << "total : " << std::accumulate(agentPrefs.begin(), agentPrefs.end(), 0) << " ";
        std::cout << std::endl;
    }
    std::vector<int> allocVec = {0, 1, 2, 1};                         // Example allocation: object 0 to agent 0, object 1 to agent 1, object 2 to agent 2, object 3 to agent 1
    Allocation alloc(numAgents, allocVec);                            // Create an allocation based on the example vector
    double utility = Utility<int>::calculateUtilityMul(prefs, alloc); // Calculate the utility of the allocation based on the preferences
    std::cout << "Utility of the allocation: " << utility << std::endl;
    return EXIT_SUCCESS;
}

int testExtendNode()
{
    int numAgents = 3;
    int numObjects = 4;
    Node node(numAgents, numObjects); // Create a node with 4 agents and 4 objects
    while (true)
    {
        Node *childNode = node.extend(); // Attempt to extend the node to create a new child node
        if (childNode != nullptr)
        {
            std::cout << "Extended node at height " << childNode->getHeight() << " with allocation: ";
            const std::vector<int> &allocVec = childNode->getCurrentAllocation().getAllocation();
            for (int object : allocVec)
            {
                std::cout << object << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cout << "No more children can be expanded from this node." << std::endl;
            break; // Exit the loop if no more children can be expanded
        }
    }

    return EXIT_SUCCESS;
}
int testMCTS()
{
    auto evalFn = [](const Preferences<int> &prefs, const Allocation &alloc, const bool verbose) -> double
    {
        return Utility<int>::calculateUtilityMul(prefs, alloc, verbose);
    };
    MCTS<int> mcts(3, 4, evalFn); // Create an MCTS instance with 3 agents and 4 objects
    const int budget = 10;        // Set the budget for the MCTS run
    mcts.run(budget);
    std::cout << "MCTS run completed." << std::endl;
    std::cout << "Get the best allocation and score from the root node: " << std::endl;
    std::pair<Allocation, Score> bestAlloc = mcts.getRoot().getBestAllocation();
    std::cout << "Best allocation: ";
    const std::vector<int> &allocVec = bestAlloc.first.getAllocation();
    for (int object : allocVec)
    {
        std::cout << object << " ";
    }
    std::cout << std::endl;
    std::cout << "Best score: " << bestAlloc.second.getScore() << std::endl;

    return EXIT_SUCCESS;
}
// src/main.cpp

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char **argv)
{
    // Force console output to UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF-8");
    /* HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE)
    {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode))
        {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    } */
#endif

    // ---------------------------------------------------------
    // STEP 1: Load base configuration (TOML file)
    // ---------------------------------------------------------
    // This will read config.toml, or create it if it doesn't exist
    MCTSConfig config = MCTSConfig::load("config.toml");

    // ---------------------------------------------------------
    // STEP 2: Terminal override (CLI11)
    // ---------------------------------------------------------
    CLI::App app{"MCTS Engine for resource allocation"};

    // Bind terminal options directly to the 'config' variables.
    // If an option is NOT passed in the terminal, the variable keeps its TOML value.
    app.add_option("-n,--num-agents", config.numAgents, "Override the number of agents");
    app.add_option("-o,--num-objects", config.numObjects, "Override the number of objects");
    app.add_option("-i,--iterations", config.iterations, "Override the number of MCTS iterations");
    app.add_option("-e,--exploration", config.exploration, "Override the exploration constant (C)");
    app.add_option("-t,--threads", config.threads, "Override the number of threads (OpenMP/TBB)");
    app.add_option("-s,--seed", config.seed, "Override the random seed for preference generation");
    app.add_flag("-l,--launch", config.launch, "Launch the interface");
    app.add_flag("-v,--verbose", config.verbose, "Enable verbose output for debugging");
    app.add_option("-r,--ratio-random", config.ratioRandom, "Override the ratio of random simulations");
    app.add_flag("-S,--save-results", config.saveResults, "Save results to a JSON file in the results directory");
    // Parse the arguments provided at launch
    // CLI11_PARSE handles errors and the help menu (-h or --help) automatically
    CLI11_PARSE(app, argc, argv);

    // ---------------------------------------------------------
    // STEP 3: Algorithm execution
    // ---------------------------------------------------------
    std::cout << "\n=== [MCTS] Starting with final configuration ===\n";
    std::cout << " - launch        : " << (config.launch ? "true" : "false") << "\n";
    std::cout << " - Num Agents    : " << config.numAgents << "\n";
    std::cout << " - Num Objects   : " << config.numObjects << "\n";
    std::cout << " - Iterations    : " << config.iterations << "\n";
    std::cout << " - Exploration C : " << config.exploration << "\n";
    std::cout << " - Threads       : " << (config.threads == -1 || config.threads > omp_get_max_threads() ? "All available" : std::to_string(config.threads)) << "\n";
    std::cout << " - Seed          : " << config.seed << "\n";
    std::cout << " - Verbose       : " << (config.verbose ? "true" : "false") << "\n";
    std::cout << " - Ratio Random Simulation : " << config.ratioRandom << "\n";
    std::cout << " - Save Results  : " << (config.saveResults ? "true" : "false") << "\n";
    std::cout << "================================================\n\n";

    // Example of how you would instantiate and run your engine:
    // MCTS mcts_engine(config.iterations, config.exploration);
    // mcts_engine.run(config.threads);
    if (config.launch)
    {
        std::cout << "Launching the interface..." << std::endl;
        // Code to launch your interface would go here
        MCTSAllocationGraph graph;
        graph.exportGraph();
        // For now, we'll just print a message and exit
        return EXIT_SUCCESS;
    }
    MCTS<int> mcts(config);

    // Print the generated preferences for debugging purposes
    // std::cout << "Generated Preferences:" << std::endl;
    // Preferences<int> &prefs = mcts.getPreferences();
    // prefs.printPreferences();

    mcts.run(config.iterations);

    // Show best allocation and score after the run
    std::cout << "Best allocation and score after MCTS run:" << std::endl;
    std::pair<Allocation, Score> bestAlloc = mcts.getRoot().getBestAllocation();
    std::cout << "Best allocation: ";
    const std::vector<int> &allocVec = bestAlloc.first.getAllocation();
    for (int object : allocVec)
    {
        std::cout << object << " ";
    }
    std::cout << "\nBest score: " << bestAlloc.second.getScore() << std::endl;

    // mcts.getEvalFunction()(mcts.getPreferences(), bestAlloc.first, true);

    if (config.saveResults)
    {
        auto t = std::time(nullptr);
        std::tm tm{};

#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        std::ostringstream oss;

        // Format : YYYY-MM-DD_HH-MM-SS
        oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
        mcts.save_results_json("mcts_results" + oss.str() + ".json");
    }
    return EXIT_SUCCESS;
}