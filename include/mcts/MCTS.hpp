#ifndef MCTS_HPP
#define MCTS_HPP
#include <memory>
#include <vector>
#include <stack>
#include <numeric>
#include <utility>
#include <cmath>
#include <sstream>
#include <functional>

#include "IMCTS.hpp"
#include "IAllocator.hpp"
#include "Node.hpp"
#include "Allocation.hpp"
#include "Score.hpp"
#include "Preferences.hpp"
#include "policies/IPolicy.hpp"
#include "policies/PolicyRegistry.hpp"
#include "policies/AllPolicies.hpp"
#include "metrics/Utility.hpp"
#include "config/config.hpp"

/**
 * @brief Main class implementing the Monte Carlo Tree Search for the allocation problem.
 * @tparam T The type of the elements being allocated (usually int)
 */
template <typename T>
class MCTS : public IAllocator<T>, public IMCTS
{
private:
    int numberOfAgents;
    int numberOfObjects;
    Node root;
    std::stack<Node *> nodeStack;
    Preferences<T> preferences;
    double explorationParameter; // Exploration parameter for UCB
    int seed;
    std::string evalFunction;         // Evaluation function to calculate scores for allocations
    std::unique_ptr<IPolicy> politic; // Pointer to a politic object for determining the ratio of random simulations
    bool verbose;
    double ratioRandom; // Ratio of random simulations to heuristic simulations
    Config config;      // Store configuration for thread access
    bool trunckateTreeSearch;
    int budgetCounter = 0;            // Counter for the number of iterations completed
    bool addMetricsToUtility = false; // Flag to determine if metrics should be added to utility
    bool workWithTimeBudget = false;  // Flag to determine if the MCTS should work with a time budget
    double timeBudgetSeconds = 60.0;  // Time budget in seconds for the MCTS search
    Allocation bestAllocation;        // Store the best allocation found during the search
    Score bestScore;                  // Store the best score found during the search

private:
    std::vector<unsigned long long> factorialCache = {1};
    int iterTrackerBestSolution = 0;       // Timer to track how many iterations the best solution hasn't changed
    unsigned long long monitoringCuts = 0; // Counter for how many times the search has been cut due to no improvement in the best solution

public:
    /**
     * @brief Default constructor initializing an empty MCTS instance.
     */
    MCTS() : numberOfAgents(0),
             numberOfObjects(0),
             root(Node()),
             nodeStack(std::stack<Node *>()),
             preferences(Preferences<T>()),
             explorationParameter(std::sqrt(2.0)),
             bestAllocation(Allocation()),
             bestScore(Score(0.0, false)),
             seed(42),
             evalFunction("MNW"),
             verbose(false),
             ratioRandom(1.0),
             config(Config()),
             trunckateTreeSearch(false),
             workWithTimeBudget(false),
             timeBudgetSeconds(60.0),
             politic(new FixedPolicy(config)) {};

    /**
     * @brief Constructor for deep initialization of MCTS.
     */
    MCTS(const int numAgents, const int numObjects, const Node root, const std::stack<Node *> nodeStack, const Preferences<T> &prefs, const double explorationParameter, const int threads, const int seed, const std::string evalFunction, const bool verbose) : numberOfAgents(numAgents),
                                                                                                                                                                                                                                                                  numberOfObjects(numObjects),
                                                                                                                                                                                                                                                                  root(root),
                                                                                                                                                                                                                                                                  nodeStack(nodeStack),
                                                                                                                                                                                                                                                                  preferences(prefs),
                                                                                                                                                                                                                                                                  explorationParameter(explorationParameter),
                                                                                                                                                                                                                                                                  seed(seed),
                                                                                                                                                                                                                                                                  evalFunction(evalFunction),
                                                                                                                                                                                                                                                                  verbose(verbose) {};

    /**
     * @brief Constructor initializing MCTS with given preferences and evaluation function.
     */
    MCTS(const int numAgents, const int numObjects, const Preferences<T> &prefs, const double explorationParameter, std::string evalFunction, const bool verbose = false) : MCTS(numAgents, numObjects, Node(numAgents, numObjects, verbose), std::stack<Node *>(), prefs, explorationParameter, 1, 42, evalFunction, verbose) {};

    /**
     * @brief Constructor initializing MCTS with randomly generated preferences.
     */
    MCTS(const int numAgents, const int numObjects, const double explorationParameter, std::string evalFunction, const bool verbose = false) : MCTS(numAgents, numObjects, Node(numAgents, numObjects, verbose), std::stack<Node *>(), Preferences<T>(numAgents, numObjects, verbose), explorationParameter, 1, 42, evalFunction, verbose) { preferences.generateRandomPreferences(numObjects * numAgents); };

    /**
     * @brief Constructor initializing MCTS with randomly generated preferences and default exploration parameter.
     */
    MCTS(const int numAgents, const int numObjects, std::string evalFunction, const bool verbose = false) : MCTS(numAgents, numObjects, Node(numAgents, numObjects, verbose), std::stack<Node *>(), Preferences<T>(numAgents, numObjects, verbose), std::sqrt(2.0), 1, 42, evalFunction, verbose) { preferences.generateRandomPreferences(numObjects * numAgents); };

    /**
     * @brief Constructor initializing MCTS using a configuration object.
     */
    MCTS(Config &config) : MCTS() { load_config(config); };

    /** @brief Get the preferences for the MCTS algorithm
     * @return Preferences<T>& The preferences for the MCTS algorithm
     */
    Preferences<T> &getPreferences() { return preferences; }

    const Preferences<T> &getPreferences() const override { return preferences; }

    /**
     * @brief Set the verbose mode
     * @param v The verbose mode
     */
    void setVerbose(bool v) { verbose = v; }

    /**
     * @brief Get the verbose mode
     * @return bool The verbose mode
     */
    bool getVerbose() const { return verbose; }

    /**
     * @brief Get if the tree search is truncated (agents must have at least one object)
     * @return bool True if the tree search is truncated, false otherwise
     */
    bool getTrunckateTreeSearch() const { return trunckateTreeSearch; }
    /**
     * @brief Get the evaluation function used to calculate scores
     * @return std::function<double(const Preferences<T> &, const Allocation &, bool)> The evaluation function
     */
    std::string getEvalFunction() const { return evalFunction; }

    /**
     * @brief Set the tree search to be truncated (agents must have at least one object)
     * @param t True to truncate the tree search, false otherwise
     */
    void setTrunckateTreeSearch(bool t) { trunckateTreeSearch = t; }

    /**
     * @brief Get the best allocation found during the MCTS search
     * @return Allocation The best allocation found
     */
    Allocation getBestAllocation() const { return bestAllocation; }

    /**
     * @brief Set the best allocation found during the MCTS search
     * @param alloc The best allocation found
     */
    void setBestAllocation(const Allocation &alloc) { bestAllocation = alloc; }

    /**
     * @brief Get the best score found during the MCTS search
     * @return Score The best score found
     */
    Score getBestScore() const { return bestScore; }

    /**
     * @brief Set the best score found during the MCTS search
     * @param s The best score found
     */
    void setBestScore(const Score &s) { bestScore = s; }
    /*----------------------------------------------*/
    /*                   Override                   */
    /*----------------------------------------------*/
    Node *getRootNode() override { return &root; }

    int getCurrentIteration() const override { return budgetCounter; }
    int getTotalIterations() const override { return config.iterations; }

    bool isWorkingWithTimeBudget() const override { return workWithTimeBudget; }
    double getTimeBudgetSeconds() const override { return timeBudgetSeconds; }

    int getNumberOfAgents() const override { return numberOfAgents; }
    int getNumberOfObjects() const override { return numberOfObjects; }

    double getExplorationParameter() const override { return explorationParameter; }
    long long getMonitoringCuts() const override { return monitoringCuts; }
    /*----------------------------------------------*/

    std::pair<Allocation, Score> solve(bool verbose = false) override
    {
        this->setVerbose(verbose);
        run(config.iterations, config.timeBudgetSeconds, config.showProgress);
        return std::make_pair(bestAllocation, bestScore);
    }
    /** @brief Runs the MCTS algorithm for a specified number of iterations.
     *  @param budget The number of iterations to run (classic budget) or the time budget in seconds (if workWithTimeBudget is true)
     *  @param showProgress Whether to show the progress bar during the run
     * @return void
     */
    void run(const int budget, const double timeBudget = 0, bool showProgress = true);

    /**
     * @brief Runs the MCTS algorithm for a specified number of iterations with a time budget
     * @param budget The number of iterations to run
     * @param showProgress Whether to show the progress bar during the run
     * @return void
     */
    void classicRun(const int budget, bool showProgress = true);

    /**
     * @brief Runs the MCTS algorithm for a specified number of iterations with a time budget
     * @param budget The time budget in seconds to run
     * @param showProgress Whether to show the progress bar during the run
     * @return void
     */
    void runWithTimeBudget(const double timeBudget, bool showProgress = true);

    /** @brief Selects a node to expand based on the UCB1 formula
     *  @param node The current node
     *  @param nodeStack The stack of nodes
     *  @return The selected node
     */
    Node *selectNode(Node *node, std::stack<Node *> *nodeStack);

    /** @brief Simulates a playout from the given node. The simulation can be either random or heuristic based on the configuration.
     *  @param node The node to simulate from
     *  @return The reward obtained from the simulation
     */
    std::pair<Allocation, Score> simulate(Node &node);

    /** @brief Backpropagates the reward obtained from a simulation up the tree
     *  @param nodeStack The stack of nodes to backpropagate through
     *  @param reward The reward to backpropagate
     *  @return std::pair<Allocation, Score> The best allocation and score found during backpropagation
     */
    std::pair<Allocation, Score> backpropagate(std::stack<Node *> &nodeStack, const std::pair<Allocation, Score> &reward);

    /**
     * @brief Loads the MCTS settings from a Config object
     * @param conf The configuration object
     */
    void load_config(const Config &conf) override
    {
        this->config = conf; // Store config for thread access
        numberOfAgents = config.numAgents;
        numberOfObjects = config.numObjects;
        explorationParameter = config.exploration;
        seed = config.seed;
        nodeStack = std::stack<Node *>();                                              // Clear the stack
        root = Node(numberOfAgents, numberOfObjects, config.verbose);                  // Reset the root node with the new number of agents and objects
        preferences = Preferences<T>(numberOfAgents, numberOfObjects, config.verbose); // Reset preferences with the new number of agents and objects
        verbose = config.verbose;
        ratioRandom = config.ratioRandom;
        preferences.generateRandomPreferences(numberOfAgents * numberOfObjects, config.seed);
        trunckateTreeSearch = config.agentHaveMinimumOneObject;
        addMetricsToUtility = config.add_metrics_to_utility;
        workWithTimeBudget = config.useTimeBudget;
        timeBudgetSeconds = config.timeBudgetSeconds;
        evalFunction = config.evalFunction;
        // TODO
        politic = PolicyRegistry::getInstance().create(config.selectedPolicy, config);
        if (politic)
        {
            politic->set_MCTS_adress(this);
        }
    }

    /**
     * @brief Converts the MCTS state and results to a JSON string
     * @param add_metrics Whether to include metrics
     * @return std::string The JSON representation
     */
    std::string to_json(const bool add_metrics = false) override
    {
        std::ostringstream oss;

        oss << "{\n";
        oss << "  \"num_agents\": " << numberOfAgents << ",\n";
        oss << "  \"num_objects\": " << numberOfObjects << ",\n";
        oss << "  \"exploration_parameter\": " << explorationParameter << ",\n";
        oss << "  \"seed\": " << seed << ",\n";
        oss << "  \"verbose\": " << (verbose ? "true" : "false") << ",\n";
        oss << "  \"ratio_random\": " << ratioRandom << ",\n";
        oss << "  \"trunckate_tree_search\": " << (trunckateTreeSearch ? "true" : "false") << ",\n";
        oss << "  \"add_metrics_to_utility\": " << (addMetricsToUtility ? "true" : "false") << ",\n";
        oss << "  \"work_with_time_budget\": " << (workWithTimeBudget ? "true" : "false") << ",\n";
        oss << "  \"time_budget_seconds\": " << timeBudgetSeconds << ",\n";
        oss << "  \"iterations\": " << (workWithTimeBudget ? this->budgetCounter : config.iterations) << ",\n";
        // add preferences matrix
        oss << "  \"preferences\": [\n";
        for (int i = 0; i < numberOfAgents; ++i)
        {
            oss << "    [";
            for (int j = 0; j < numberOfObjects; ++j)
            {
                oss << preferences.getPreference(i, j);
                if (j < numberOfObjects - 1)
                    oss << ", ";
            }
            oss << "]";
            if (i < numberOfAgents - 1)
                oss << ",\n";
        }

        oss << "  ],\n";
        // add best allocation and score
        oss << "  \"best_allocation\": [";
        const std::vector<int> &allocVec = this->getBestAllocation().getAllocation();
        for (size_t i = 0; i < allocVec.size(); ++i)
        {
            oss << allocVec[i];
            if (i < allocVec.size() - 1)
                oss << ", ";
        }
        oss << "],\n";
        oss << "  \"best_score\": " << this->getBestScore().getScore() << ",\n";
        oss << "  \"metrics\": {\n";
        if (add_metrics)
        {
            for (const auto &[name, metricFunc] : getMetricsRegistry<T>())
            {
                double metricValue = metricFunc(getPreferences(), getBestAllocation());
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
    /**
     * @brief Saves the JSON results to a file
     * @param filename The output filename
     * @param add_metrics Whether to include metrics in the output
     */
    void save_results_json(const std::string &filename, const bool add_metrics = false) override
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

    /** \brief Calculate the factorial of a number
     * \param n The number to calculate the factorial of
     * \return The factorial of n
     */
    unsigned long long factorial(int n)
    {
        if (n < 0 || n > 20)
            return 0;

        // Initialize the cache with the first factorial value if it's empty
        if (factorialCache.empty())
        {
            factorialCache.push_back(1);
        }

        // Case 1: The value is already in the cache, return it directly
        if (n < factorialCache.size())
        {
            return factorialCache[n];
        }
        // Case 2: The value is not in the cache, compute it and store it in the cache
        else
        {
            unsigned int oldSize = factorialCache.size();
            factorialCache.resize(n + 1);

            // Compute the factorial values from the last cached value up to n and store them in the cache
            for (unsigned int i = oldSize; i <= n; ++i)
            {
                factorialCache[i] = i * factorialCache[i - 1];
            }

            return factorialCache[n];
        }
    }

    /**
     * @brief Clears the internal state of the MCTS tree, including the root node and memory.
     */
    void clear() override
    {
        root = Node(numberOfAgents, numberOfObjects, verbose);
        nodeStack = std::stack<Node *>();
        preferences.clear();
        monitoringCuts = 0;
        budgetCounter = 0;
        bestAllocation = Allocation();
        Score bestScore = Score();
    }
};

#endif // MCTS_HPP