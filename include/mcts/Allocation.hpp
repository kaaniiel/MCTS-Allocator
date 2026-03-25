#ifndef ALLOCATION_HPP
#define ALLOCATION_HPP
#include <vector>

class Allocation
{
private:
    int numObjects; // Number of objects to allocate, needed for generating children
    int numAgents;  // Number of agents, needed for generating children
    // A vector for each agent: ex 3 agents & 4object: [0,1,2,1] means agent 0 gets object 0, agent 1 gets object 1 and 3, agent 2 gets object 2]
    std::vector<int> allocation;

public:
    Allocation() = default;
    // Constructor to initialize the allocation with the number of objects and agents
    Allocation(const int numAgents, const int numObjects) : numObjects(numObjects),
                                                            numAgents(numAgents),
                                                            allocation(numObjects, -1) {}; // Initialize with -1 to indicate unallocated objects

    Allocation(const int numAgents, const std::vector<int> &alloc) : numObjects(alloc.size()),
                                                                     numAgents(numAgents),
                                                                     allocation(alloc) {};

    /**
     * @brief Get the allocation vector
     * @return const std::vector<int>& The allocation vector
     */
    const std::vector<int> &getAllocation() const { return allocation; };

    /**
     * @brief Set the allocation vector
     * @param newAllocation The new allocation vector to set
     * @return void
     */
    void setAllocation(const std::vector<int> &newAllocation) { allocation = newAllocation; };

    /**
     * @brief Get the number of objects
     * @return int The number of objects
     */
    int getNumObjects() const { return numObjects; };
    /**
     * @brief Get the number of agents
     * @return int The number of agents
     */
    int getNumAgents() const { return numAgents; };
};

#endif // ALLOCATION_HPP