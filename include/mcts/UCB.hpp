#ifndef UCB_HPP
#define UCB_HPP

#include "Node.hpp"

class UCB
{
public:
    /**
     * @brief Calculate the UCB1 value for a given node
     * @param node The node for which to calculate the UCB1 value
     * @param explorationParameter The exploration parameter (commonly denoted as 'c' in the UCB1 formula)
     * @return double The calculated UCB1 value
     */
    static double calculate(const Node &node, const double explorationParameter);
};

#endif // UCB_HPP
