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

Node *UCB::selectBestChild(Node *node, const double explorationParameter)
{
    if (node == nullptr)
    {
        return nullptr;
    }

    if (node->getChildren().empty())
    {
        return node; // Return the current node if it is not fully expanded
    }

    if (node->getChildren().size() < static_cast<size_t>(node->getNumAgents()))
    {
        return node;
    }

    Node *bestChild = &node->getChildren().front();
    double bestValue = -std::numeric_limits<double>::infinity();

    for (auto &child : node->getChildren())
    {
        double ucbValue = calculate(child, explorationParameter);
        if (ucbValue > bestValue)
        {
            bestValue = ucbValue;
            bestChild = &child;
        }
    }

    return bestChild;
}