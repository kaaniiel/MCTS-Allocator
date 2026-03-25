#ifndef MCTS_HPP
#define MCTS_HPP
#include <memory>
#include <vector>
#include <stack>
#include <utility>

#include "Node.hpp"
#include "Allocation.hpp"
#include "Score.hpp"

class MCTS
{
private:
    int numberOfAgents;
    int numberOfObjects;
    Node root;
    std::stack<Node> nodeStack;

public:
    MCTS(const int numAgents, const int numObjects) : numberOfAgents(numAgents), numberOfObjects(numObjects), root(Node()) {}

    /** @brief Runs the MCTS algorithm for a specified number of iterations
     *  @param iterations The number of iterations to run
     * @return void
     */
    void run(const int iterations);

    /** @brief Selects a node to expand based on the UCB1 formula
     *  @param node The current node
     *  @param nodeStack The stack of nodes
     *  @return The selected node
     */
    Node selectNode(const Node &node, std::stack<Node> &nodeStack);

    /** @brief Expands a node by generating one of its unvisited children
     *  @param node The node to expand
     *  @return The expanded node
     */
    Node extendNode(const Node &node);

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
    std::pair<Allocation, Score> backpropagate(std::stack<Node> &nodeStack, const std::pair<Allocation, Score> &reward);
};

#endif // MCTS_HPP