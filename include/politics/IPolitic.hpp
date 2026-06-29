#ifndef IPOLITIC_HPP
#define IPOLITIC_HPP
#include "mcts/Allocation.hpp"
class IPolitic
{
public:
    virtual ~IPolitic() = default;

    virtual double get_ratio_limit(const Allocation currentAllocation) = 0;
};
#endif // IPOLITIC_HPP