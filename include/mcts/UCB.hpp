#ifndef UCB_HPP
#define UCB_HPP

#include "Node.hpp"

/**
 * @brief Utility class for UCB1 (Upper Confidence Bound) calculations and selections.
 */
class UCB
{
public:
    /**
     * @brief Calculate the UCB1 value for a given node
     * @param node The node for which to calculate the UCB1 value
     * @param explorationParameter The exploration parameter (commonly denoted as 'c' in the UCB1 formula)
     * @return double The calculated UCB1 value
     */
    static double calculate(const Node &node, const int parentVisits, const double explorationParameter, const bool verbose = false);

    /**
     * @brief Select the best child node based on the UCB1 value
     * @param node The parent node from which to select the best child
     * @param explorationParameter The exploration parameter to use in the UCB1 calculation
     * @return Node* The child node with the highest UCB1 value, or the current node if it was not fully expanded
     */
    static Node *selectBestChild(Node *node, const double explorationParameter, const bool verbose = false);
};

#endif // UCB_HPP
