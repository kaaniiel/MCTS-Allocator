#ifndef ALLOCATION_HPP
#define ALLOCATION_HPP
#include <vector>
#include <stdexcept>

/**
 * @brief Class representing an allocation of objects to agents
 */
class Allocation
{
private:
    int numObjects; // Number of objects to allocate, needed for generating children
    int numAgents;  // Number of agents, needed for generating children
    // A vector for each agent: ex 3 agents & 4object: [0,1,2,1] means agent 0 gets object 0, agent 1 gets object 1 and 3, agent 2 gets object 2]
    std::vector<int> allocation;
    bool verbose;

public:
    /**
     * @brief Default constructor for Allocation
     */
    Allocation() = default;

    /**
     * @brief Constructor to initialize the allocation with the number of objects and agents
     * @param numAgents Number of agents
     * @param numObjects Number of objects
     * @param verbose Whether to enable verbose mode
     */
    Allocation(const int numAgents, const int numObjects, const bool verbose = false) : numObjects(numObjects),
                                                                                        numAgents(numAgents),
                                                                                        allocation(numObjects, -1),
                                                                                        verbose(verbose) {}; // Initialize with -1 to indicate unallocated objects

    /**
     * @brief Constructor from an existing allocation vector
     * @param numAgents Number of agents
     * @param alloc The allocation vector to copy
     * @param verbose Whether to enable verbose mode
     */
    Allocation(const int numAgents, const std::vector<int> &alloc, const bool verbose = false) : numObjects(alloc.size()),
                                                                                                 numAgents(numAgents),
                                                                                                 allocation(alloc),
                                                                                                 verbose(verbose) {};

    /**
     * @brief Move constructor for efficiency
     * @param numAgents Number of agents
     * @param alloc The allocation vector to move
     * @param verbose Whether to enable verbose mode
     */
    Allocation(const int numAgents, std::vector<int> &&alloc, const bool verbose = false) : numObjects(alloc.size()),
                                                                                            numAgents(numAgents),
                                                                                            allocation(std::move(alloc)),
                                                                                            verbose(verbose) {};

    /**
     * @brief Get the allocation vector
     * @return const std::vector<int>& The allocation vector
     */
    const std::vector<int> &getAllocation() const { return allocation; };

    /**
     * @brief Get the agent allocated to a specific object
     * @param objectIndex The index of the object
     * @return const int The index of the agent
     * @throws std::out_of_range If objectIndex is invalid
     */
    const int getAgentForObject(int objectIndex) const
    {
        if (objectIndex < 0 || objectIndex >= numObjects)
        {
            throw std::out_of_range("Object index out of range");
        }
        return allocation[objectIndex];
    }
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

    /**
     * @brief Set the verbose mode
     * @param v The verbose mode
     */
    void setVerbose(bool v) { verbose = v; }

    /**
     * @brief Get the verbose mode
     * @return bool The verbose mode
     */
    bool getVerbose() const { return verbose; }

    /**
     * @brief Release internal memory used by allocation.
     */
    void clear()
    {
        allocation.clear();
        allocation.shrink_to_fit();
        numObjects = 0;
        numAgents = 0;
        verbose = false;
    }
};

#endif // ALLOCATION_HPP