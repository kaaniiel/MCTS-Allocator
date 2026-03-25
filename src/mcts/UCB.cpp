#include "mcts/UCB.hpp"

#include <cmath>
#include <limits>
double UCB::calculate(const Node &node, const double explorationParameter)
{
    // if the node has not been visited yet, return infinity to ensure it gets selected
    if (node.getVisits() == 0)
    {
        return std::numeric_limits<double>::infinity();
    }

    // Calculate the average reward (exploitation term)
    double averageReward = node.getBestAllocation().second.getScores()[0] / node.getVisits();
    // Calculate the exploration term
    double explorationTerm = explorationParameter * std::sqrt(std::log(node.getVisits()) / node.getVisits());
    // Return the sum of the exploitation and exploration terms
    return averageReward + explorationTerm;
}

Node UCB::selectBestChild(const Node &node, const double explorationParameter)
{
    if (node.getChildren().empty())
    {
        return node; // Return the current node if it has no children
    }

    if (node.getChildren().size() != node.getNumAgents())
    {
        return node; // Return the current node if it is not fully expanded
    }

    Node bestChild;
    double bestValue = -std::numeric_limits<double>::infinity();

    for (const auto &child : node.getChildren())
    {
        double ucbValue = calculate(child, explorationParameter);
        if (ucbValue > bestValue)
        {
            bestValue = ucbValue;
            bestChild = child;
        }
    }

    return bestChild;
}