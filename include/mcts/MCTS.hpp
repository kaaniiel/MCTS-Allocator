#ifndef MCTS_HPP
#define MCTS_HPP
#include <memory>
#include <vector>
#include <stack>
#include <utility>
#include <cmath>

#include "Node.hpp"
#include "Allocation.hpp"
#include "Score.hpp"
#include "Preferences.hpp"

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

public:
    MCTS(const int numAgents, const int numObjects, const Node root, const std::stack<Node *> nodeStack, const Preferences<T> &prefs, const double explorationParameter) : numberOfAgents(numAgents),
                                                                                                                                                                           numberOfObjects(numObjects),
                                                                                                                                                                           root(root),
                                                                                                                                                                           nodeStack(nodeStack),
                                                                                                                                                                           preferences(prefs),
                                                                                                                                                                           explorationParameter(explorationParameter) {};
    MCTS(const int numAgents, const int numObjects, const Preferences<T> &prefs, const double explorationParameter) : MCTS(numAgents, numObjects, Node(numAgents, numObjects), std::stack<Node *>(), prefs, explorationParameter) {};
    MCTS(const int numAgents, const int numObjects, const double explorationParameter) : MCTS(numAgents, numObjects, Node(numAgents, numObjects), std::stack<Node *>(), Preferences<T>(numAgents, numObjects), explorationParameter) { preferences.generateRandomPreferences(numObjects * numAgents); };
    MCTS(const int numAgents, const int numObjects) : MCTS(numAgents, numObjects, Node(numAgents, numObjects), std::stack<Node *>(), Preferences<T>(numAgents, numObjects), std::sqrt(2.0)) { preferences.generateRandomPreferences(numObjects * numAgents); };

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

    /** @brief Runs the MCTS algorithm for a specified number of iterations
     *  @param budget The number of iterations to run
     * @return void
     */
    void run(const int budget);

    /**
     * @brief Runs the MCTS algorithm in parallel for a specified number of iterations
     * @param budget The number of iterations to run
     * @return void
     */
    void parallelRun(const int budget);
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
    std::pair<Allocation, Score> simulate(const Node &node);

    /** @brief Backpropagates the reward obtained from a simulation up the tree
     *  @param nodeStack The stack of nodes to backpropagate through
     *  @param reward The reward to backpropagate
     *  @return std::pair<Allocation, Score> The best allocation and score found during backpropagation
     */
    std::pair<Allocation, Score> backpropagate(std::stack<Node *> &nodeStack, const std::pair<Allocation, Score> &reward);
};

#endif // MCTS_HPP