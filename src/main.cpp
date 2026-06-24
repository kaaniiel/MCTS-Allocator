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
#include "Solver.hpp"

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

void setConsoleToUTF8()
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
}

template <typename T>
void show_metrics(const Preferences<T> &prefs, const Allocation &alloc)
{
    for (const auto &[name, metricFunc] : getMetricsRegistry<T>())
    {
        std::cout << " - " << name << ": " << (metricFunc(prefs, alloc) ? "Yes" : "No") << std::endl;
    }
}

/**
 * @brief Display the final configuration settings to the user.
 * @param config The configuration object containing the settings.
 */
void showOptions(Config config)
{
    // ---------------------------------------------------------
    // STEP 3: Algorithm execution
    // ---------------------------------------------------------

    std::cout << "\n=== [MCTS] Starting with final configuration ===\n";
    std::cout << " - launch        : " << (config.launch ? "true" : "false") << "\n";
    std::cout << " - Num Agents    : " << config.numAgents << "\n";
    std::cout << " - Num Objects   : " << config.numObjects << "\n";
    std::cout << " - Iterations    : " << config.iterations << "\n";
    std::cout << " - Exploration C : " << config.exploration << "\n";
    std::cout << " - Seed          : " << config.seed << "\n";
    std::cout << " - Verbose       : " << (config.verbose ? "true" : "false") << "\n";
    std::cout << " - Ratio Random  : " << config.ratioRandom << "\n";
    std::cout << " - Save Results  : " << (config.saveResults ? "true" : "false") << "\n";
    std::cout << " - Use Solver    : " << (config.useSolver ? "true" : "false") << "\n";
    std::cout << " - Monitoring Cuts : " << (config.monitoringCuts ? "true" : "false") << "\n";
    std::cout << " - Uniformize Negative Values : " << (config.uniformizeNegativeValues ? "true" : "false") << "\n";
    std::cout << " - Agent Have Minimum One Object : " << (config.agentHaveMinimumOneObject ? "true" : "false") << "\n";
    std::cout << " - Add Metrics to Utility : " << (config.add_metrics_to_utility ? "true" : "false") << "\n";
    std::cout << " - Use Time Budget : " << (config.useTimeBudget ? "true" : "false") << "\n";
    std::cout << " - Time Budget Seconds : " << config.timeBudgetSeconds << "\n";
    std::cout << " - Terminal JSON Output : " << (config.terminalJSONOutput ? "true" : "false") << "\n";
    std::cout << " - Show Metrics : " << (config.show_metrics ? "true" : "false") << "\n";
    std::cout << " - Show Progress : " << (config.showProgress ? "true" : "false") << "\n";
    std::cout << "================================================\n\n";
}

/**
 * @brief Parse command-line arguments and override configuration settings.
 * @param config Reference to the configuration object.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @param showOptionsOutput Whether to display options output.
 * @return Exit status.
 */
int CLI_conf(Config &config, int argc, char **argv, bool showOptionsOutput = true)
{
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
    app.add_option("-s,--seed", config.seed, "Override the random seed for preference generation");
    app.add_option("-r,--ratio-random", config.ratioRandom, "Override the ratio of random simulations");
    app.add_option("-t, --time-budget-seconds", config.timeBudgetSeconds, "Override the time budget in seconds for MCTS");
    app.add_flag("-A,--agent-have-minimum-one-object", config.agentHaveMinimumOneObject, "Ensure each agent has at least one object in the allocation");
    app.add_flag("-B, --use-time-budget", config.useTimeBudget, "Use a time budget instead of a number of iterations for MCTS");
    app.add_flag("-G, --show-metrics", config.show_metrics, "Show metrics (EF, EFX, Prop, ...) for the best allocation after MCTS run");
    app.add_flag("-J, --terminal-json-output", config.terminalJSONOutput, "Output results in JSON format to the terminal");
    app.add_flag("-L,--launch", config.launch, "Launch the interface");
    app.add_flag("-M,--monitoring-cuts", config.monitoringCuts, "Enable monitoring of cuts when the best solution hasn't improved for a certain number of iterations");
    app.add_flag("-N,--uniformize-negative-values", config.uniformizeNegativeValues, "Uniformize negative values in preferences (transform to how much agent hasn't received an object)");
    app.add_flag("-S,--save-results", config.saveResults, "Save results to a JSON file in the results directory");
    app.add_flag("-T,--add-metrics-to-utility", config.add_metrics_to_utility, "Add metrics to the utility calculation (EF, EFX, Prop, etc.)");
    app.add_flag("-U,--use-solver", config.useSolver, "Use the Gurobi solver to find the optimal allocation instead of MCTS");
    app.add_flag("-V,--verbose", config.verbose, "Enable verbose output for debugging");
    app.add_flag("-P,--show-progress", config.showProgress, "Show progress information during the MCTS run");
    // Parse the arguments provided at launch
    // CLI11_PARSE handles errors and the help menu (-h or --help) automatically
    try
    {
        app.parse(argc, argv);
        // On n'affiche les options que si on ne veut pas une sortie purement JSON
        if (showOptionsOutput && !config.terminalJSONOutput)
        {
            showOptions(config);
        }
        return EXIT_SUCCESS;
    }
    catch (const CLI::ParseError &e)
    {
        // Intercepte les erreurs (ou l'affichage de l'aide -h)
        // et quitte proprement le programme entier sans retourner au main().
        std::exit(app.exit(e));
    }
}

/**
 * @brief Launch the interface for the MCTS allocation graph visualization.
 * This function initializes the MCTSAllocationGraph and exports the graph to the default system viewer.
 */
void launch_interface()
{
    std::cout << "Launching the interface..." << std::endl;
    // Code to launch your interface would go here
    MCTSAllocationGraph graph;
    graph.exportGraph();
    // For now, we'll just print a message and exit
}

/**
 * @brief Prepare a file name with a timestamp.
 * @param prefix The prefix for the file name.
 * @param suffix The suffix for the file name.
 * @return The prepared file name.
 */
std::string prepare_file_name(std::string prefix, std::string suffix)
{
    auto t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return prefix + oss.str() + suffix;
}

int main(int argc, char **argv)
{
    setConsoleToUTF8(); // Set console to UTF-8 for proper character display

    // ---------------------------------------------------------
    // STEP 1: Load base configuration (TOML file)
    // ---------------------------------------------------------
    Config config;
    try
    {
        config = Config::load("config.toml");
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        const std::string message = ex.what();
        if (message.find("Missing configuration file") != std::string::npos)
        {
            Config::generate_default("config.toml", config);
            std::cerr << "A default configuration file has been generated. Please review and modify 'config.toml' as needed, then re-run the program." << std::endl;
        }
        else
        {
            std::cerr << "'config.toml' has been rewritten with the missing values filled in. Please review the result and re-run the program." << std::endl;
        }
        return EXIT_FAILURE;
    }

    CLI_conf(config, argc, argv); // Override config values with command-line arguments

    // Safety: do not attempt to search for a solution when there are fewer objects than agents.
    if (config.numObjects < config.numAgents)
    {
        std::cerr << "Error: number of objects (" << config.numObjects << ") is less than number of agents (" << config.numAgents << "). Aborting search.\n";
        return EXIT_FAILURE;
    }

    // Example of how you would instantiate and run your engine:
    // MCTS mcts_engine(config.iterations, config.exploration);
    // mcts_engine.run(config.threads);
    if (config.launch)
    {
        launch_interface();
        return EXIT_SUCCESS;
    }

    std::unique_ptr<IAllocator<int>> allocator;
    std::string filePrefix;

    if (config.useSolver)
    {
        if (!config.terminalJSONOutput)
            std::cout << "Using the Gurobi solver to find the optimal allocation..." << std::endl;
        allocator = std::make_unique<Solver<int>>(config);
        filePrefix = "solver_results_";
    }
    else
    {
        if (!config.terminalJSONOutput)
            std::cout << "Using the MCTS engine to find the allocation..." << std::endl;
        allocator = std::make_unique<MCTS<int>>(config);
        filePrefix = "mcts_results_";

        if (!config.terminalJSONOutput && config.verbose)
        {
            allocator->getPreferences().printPreferences();
        }
    }

    // =========================================================
    // ÉTAPE 2 : Appel uniformisé de la résolution
    // =========================================================
    std::pair<Allocation, Score> bestAlloc = allocator->solve(config.verbose && !config.terminalJSONOutput);

    // =========================================================
    // ÉTAPE 3 : Affichages classiques (désactivés si -J)
    // =========================================================
    if (!config.terminalJSONOutput)
    {
        std::cout << "\n=== [Results] Best allocation and score found ===" << std::endl;
        std::cout << "Best allocation: ";
        const std::vector<int> &allocVec = bestAlloc.first.getAllocation();
        for (int object : allocVec)
            std::cout << object << " ";
        std::cout << "\nBest score: " << bestAlloc.second.getScore() << std::endl;

        if (!config.useSolver && config.monitoringCuts)
        {
            if (auto mctsPtr = dynamic_cast<MCTS<int> *>(allocator.get()))
                std::cout << "Number of cuts inside the MCTS search: " << mctsPtr->getMonitoringCuts() << std::endl;
        }

        if (config.show_metrics)
        {
            std::cout << "Metrics for the optimal allocation:" << std::endl;
            show_metrics(allocator->getPreferences(), bestAlloc.first);
        }
    }

    // =========================================================
    // ÉTAPE 4 : Sortie Fichier et/ou Terminal JSON
    // =========================================================
    if (config.saveResults)
    {
        allocator->save_results_json(prepare_file_name(filePrefix, ".json"), config.show_metrics);
    }

    if (config.terminalJSONOutput)
    {
        // On affiche UNIQUEMENT le JSON généré par l'algorithme
        std::cout << allocator->to_json(config.show_metrics) << std::endl;
    }

    return EXIT_SUCCESS;
}
