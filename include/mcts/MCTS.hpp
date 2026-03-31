#ifndef MCTS_HPP
#define MCTS_HPP
#include <memory>
#include <vector>
#include <stack>
#include <utility>
#include <cmath>
#include <functional>

#include "Node.hpp"
#include "Allocation.hpp"
#include "Score.hpp"
#include "Preferences.hpp"
#include "metrics/Utility.hpp"
#include "config/config.hpp"

template <typename T>
class MCTS
{
private:
    int numberOfAgents;
    int numberOfObjects;
    Node root;
    std::stack<Node *> nodeStack;
    Preferences<T> preferences;
    double explorationParameter; // Exploration parameter for UCB
    int numThreads;              // Number of threads for parallel execution
    int seed;
    std::function<std::vector<double>(const Preferences<T> &prefs, const Allocation &alloc, const bool verbose)> evalFunction; // Evaluation function to calculate scores for allocations
    bool verbose;

public:
    MCTS() : numberOfAgents(0),
             numberOfObjects(0),
             root(Node()),
             nodeStack(std::stack<Node *>()),
             preferences(Preferences<T>()),
             explorationParameter(std::sqrt(2.0)),
             numThreads(1),
             seed(42),
             evalFunction(Utility<T>::calculateUtilityMul),
             verbose(false) {};
    MCTS(const int numAgents, const int numObjects, const Node root, const std::stack<Node *> nodeStack, const Preferences<T> &prefs, const double explorationParameter, const int threads, const int seed, const std::function<std::vector<double>(const Preferences<T> &prefs, const Allocation &alloc, const bool verbose)> evalFunction, const bool verbose) : numberOfAgents(numAgents),
                                                                                                                                                                                                                                                                                                                                                                   numberOfObjects(numObjects),
                                                                                                                                                                                                                                                                                                                                                                   root(root),
                                                                                                                                                                                                                                                                                                                                                                   nodeStack(nodeStack),
                                                                                                                                                                                                                                                                                                                                                                   preferences(prefs),
                                                                                                                                                                                                                                                                                                                                                                   explorationParameter(explorationParameter),
                                                                                                                                                                                                                                                                                                                                                                   numThreads(threads),
                                                                                                                                                                                                                                                                                                                                                                   seed(seed),
                                                                                                                                                                                                                                                                                                                                                                   evalFunction(evalFunction),
                                                                                                                                                                                                                                                                                                                                                                   verbose(verbose) {};
    MCTS(const int numAgents, const int numObjects, const Preferences<T> &prefs, const double explorationParameter, const std::function<std::vector<double>(const Preferences<T> &prefs, const Allocation &alloc, const bool verbose)> evalFunction, const bool verbose = false) : MCTS(numAgents, numObjects, Node(numAgents, numObjects, verbose), std::stack<Node *>(), prefs, explorationParameter, 1, 42, evalFunction, verbose) {};
    MCTS(const int numAgents, const int numObjects, const double explorationParameter, const std::function<std::vector<double>(const Preferences<T> &prefs, const Allocation &alloc, const bool verbose)> evalFunction, const bool verbose = false) : MCTS(numAgents, numObjects, Node(numAgents, numObjects, verbose), std::stack<Node *>(), Preferences<T>(numAgents, numObjects, verbose), explorationParameter, 1, 42, evalFunction, verbose) { preferences.generateRandomPreferences(numObjects * numAgents); };
    MCTS(const int numAgents, const int numObjects, const std::function<std::vector<double>(const Preferences<T> &prefs, const Allocation &alloc, const bool verbose)> evalFunction, const bool verbose = false) : MCTS(numAgents, numObjects, Node(numAgents, numObjects, verbose), std::stack<Node *>(), Preferences<T>(numAgents, numObjects, verbose), std::sqrt(2.0), 1, 42, evalFunction, verbose) { preferences.generateRandomPreferences(numObjects * numAgents); };
    MCTS(MCTSConfig &config) : MCTS() { loadConfig(config); };
    /**
     * @brief Get the number of objects
     * @return int The number of objects
     */
    int getNumberOfObjects() const { return numberOfObjects; }

    /**
     * @brief Get the number of agents
     * @return int The number of agents
     */
    int getNumberOfAgents() const { return numberOfAgents; }

    /**
     * @brief Get the root node of the MCTS tree
     * @return const Node& The root node of the MCTS tree
     */
    const Node &getRoot() const { return root; }

    /** @brief Get the preferences for the MCTS algorithm
     * @return Preferences<T>& The preferences for the MCTS algorithm
     */
    Preferences<T> &getPreferences() { return preferences; }

    /**
     * @brief Get the exploration parameter for the UCB formula
     * @return double The exploration parameter for the UCB formula
     */
    double getExplorationParameter() const { return explorationParameter; }

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

    /** @brief Runs the MCTS algorithm for a specified number of iterations
     *  @param budget The number of iterations to run
     * @return void
     */
    void run(const int budget);

    /** @brief Selects a node to expand based on the UCB1 formula
     *  @param node The current node
     *  @param nodeStack The stack of nodes
     *  @return The selected node
     */
    Node *selectNode(Node *node, std::stack<Node *> *nodeStack);

    /** @brief Simulates a random playout from the given node
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
     * @brief Load configuration from a Config struct
     * @param config The configuration to load
     * @return void
     */
    void loadConfig(MCTSConfig &config)
    {

        numberOfAgents = config.numAgents;
        numberOfObjects = config.numObjects;
        explorationParameter = config.exploration;
        numThreads = config.threads;
        seed = config.seed;
        nodeStack = std::stack<Node *>();                                              // Clear the stack
        root = Node(numberOfAgents, numberOfObjects, config.verbose);                  // Reset the root node with the new number of agents and objects
        preferences = Preferences<T>(numberOfAgents, numberOfObjects, config.verbose); // Reset preferences with the new number of agents and objects
        verbose = config.verbose;

        preferences.generateRandomPreferences(numberOfAgents * numberOfObjects, config.seed);
    }
};

#endif // MCTS_HPP