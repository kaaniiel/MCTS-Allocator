#ifndef UCB_HPP
#define UCB_HPP

#include "Node.hpp"

class UCB
{
public:
    static double calculate(const Node &node, const double explorationParameter);
};

#endif // UCB_HPP
