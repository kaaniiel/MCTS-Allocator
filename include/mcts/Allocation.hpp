#ifndef ALLOCATION_HPP
#define ALLOCATION_HPP
#include <vector>

class Allocation
{
private:
    // A vector for each agent: ex 3 agents & 4object: [0,1,2,1] means agent 0 gets object 0, agent 1 gets object 1 and 3, agent 2 gets object 2]
    std::vector<int> allocation;

public:
    Allocation() = default;
    Allocation(const std::vector<int> &alloc) : allocation(alloc) {};

    /**
     * @brief Get the allocation vector
     * @return const std::vector<int>& The allocation vector
     */
    const std::vector<int> &getAllocation() const { return allocation; };
};

#endif // ALLOCATION_HPP