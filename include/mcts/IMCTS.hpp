#ifndef IMCTS_HPP
#define IMCTS_HPP

// Une interface pure qui ne contient AUCUN template
#include "Node.hpp"
/**
 * @brief A pure interface for MCTS without template arguments
 */
class IMCTS
{
public:
    virtual ~IMCTS() = default;

    /**
     * @brief Get the root node of the MCTS tree
     * @return Node* Pointer to the root node
     */
    virtual Node *getRootNode() = 0;

    /**
     * @brief Get the current iteration number of the MCTS process
     * @return int The current iteration
     */
    virtual int getCurrentIteration() const = 0;

    /**
     * @brief Get the total number of iterations planned for the MCTS process
     * @return int The total iterations
     */
    virtual int getTotalIterations() const = 0;

    /**
     * @brief Check if the MCTS is running with a time budget instead of iteration count
     * @return true if using time budget, false otherwise
     */
    virtual bool isWorkingWithTimeBudget() const = 0;

    /**
     * @brief Get the time budget in seconds if time budget mode is active
     * @return double The time budget in seconds
     */
    virtual double getTimeBudgetSeconds() const = 0;

    /**
     * @brief Get the number of agents in the allocation problem
     * @return int The number of agents
     */
    virtual int getNumberOfAgents() const = 0;

    /**
     * @brief Get the number of objects to allocate
     * @return int The number of objects
     */
    virtual int getNumberOfObjects() const = 0;

    /**
     * @brief Get the exploration parameter (C) used in UCB calculations
     * @return double The exploration parameter
     */
    virtual double getExplorationParameter() const = 0;

    /**
     * @brief Get the number of cuts made by the MCTS during execution
     * @return long long The number of monitoring cuts
     */
    virtual long long getMonitoringCuts() const = 0;
};

#endif // IMCTS_HPP