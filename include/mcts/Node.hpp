#ifndef NODE_HPP
#define NODE_HPP

#include <iostream>
#include <vector>
#include <random>
#include <ctime>

#include "Allocation.hpp"
#include "Score.hpp"

/**
 * @brief Represents a node in the Monte Carlo Tree Search.
 * A node holds an allocation state and information about its tree position.
 */
class Node
{
private:
    std::vector<bool> agentHasObject; // To keep track of which agents have been allocated an object, needed for generating children
    int visits;
    int h;             // height of the node in the tree
    int childrenIndex; // index of the next child to expand
    Allocation currentAllocation;
    std::pair<Allocation, Score> bestAllocation;
    std::vector<Node> children;

public:
    /**
     * @brief Default constructor for Node
     */
    Node() : children(),
             agentHasObject(0, false),
             visits(0),
             h(0),
             childrenIndex(0),
             currentAllocation(Allocation()),
             bestAllocation(Allocation(), Score())

    {
    };
    /**
     * @brief Constructor to initialize a Node with given number of agents and objects
     * @param numAgents Number of agents
     * @param numObjects Number of objects
     * @param verbose Whether to enable verbose mode
     */
    Node(const int numAgents, const int numObjects, const bool verbose = false) : visits(0),
                                                                                  h(0),
                                                                                  childrenIndex(0),
                                                                                  currentAllocation(Allocation(numAgents, numObjects, verbose)),
                                                                                  bestAllocation(Allocation(numAgents, numObjects, verbose), Score(0.0, verbose))
    {
        bestAllocation.first.setAllocation(currentAllocation.getAllocation());
        bestAllocation.second.setScore(0.0);

        // On peut toujours utiliser numAgents ici car il est passé en paramètre !
        children.reserve(numAgents);
    };

    /**
     * @brief Copy constructor for Node
     * @param other The node to copy
     */
    Node(const Node &other) : visits(other.visits),
                              h(other.h),
                              childrenIndex(0),
                              currentAllocation(other.currentAllocation),
                              bestAllocation(other.bestAllocation)
    {
        // On récupère le nombre d'agents depuis l'allocation pour le reserve
        children.reserve(currentAllocation.getNumAgents());
    };

    /**
     * @brief Constructor to initialize a Node with a specific allocation
     * @param alloc The initial allocation for this node
     */
    Node(const Allocation &alloc) : visits(0),
                                    h(0),
                                    childrenIndex(0),
                                    currentAllocation(alloc),
                                    bestAllocation(alloc, Score(0.0, alloc.getVerbose()))
    {
        children.reserve(alloc.getNumAgents());
    };

    /**
     * @brief Constructor to initialize a Node with a specific allocation and height
     * @param alloc The initial allocation for this node
     * @param height The depth of the node in the tree
     * @param verbose Whether to enable verbose mode
     */
    Node(const Allocation &alloc, int height, const bool verbose = false) : visits(0),
                                                                            h(height),
                                                                            childrenIndex(0),
                                                                            currentAllocation(alloc),
                                                                            bestAllocation(alloc, Score(0.0, verbose))
    {
        children.reserve(alloc.getNumAgents());
    };

    /**
     * @brief Get the agents that have not been allocated an object
     * @return std::vector<int> The agents that have not been allocated an object
     */
    std::vector<int> getAgentWithoutObject(int numAgents) const
    {
        std::vector<int> agentsWithoutObject;
        std::vector<bool> hasObject(numAgents, false);
        const std::vector<int> &allocVec = currentAllocation.getAllocation();

        for (int object = 0; object < static_cast<int>(allocVec.size()); ++object)
        {
            const int agent = allocVec[object];
            if (agent >= 0 && agent < numAgents)
            {
                hasObject[agent] = true;
            }
        }

        for (int i = 0; i < numAgents; i++)
        {
            if (!hasObject[i])
            {
                agentsWithoutObject.push_back(i);
            }
        }
        return agentsWithoutObject;
    }

    /**
     * @brief Get the number of objects that have not been allocated
     * @return int The number of objects that have not been allocated
     */
    int getObjectsNotAllocated(int numObjects) const
    {
        // the height of the node in the tree corresponds to the number
        // of allocated objects, so the number of unallocated objects is
        // numObjects - h
        return numObjects - h;
    }

    /**
     * @brief Get the agents that can be selected when truncation is enabled.
     * @return std::vector<int> The agents that can be expanded from this node.
     */
    std::vector<int> getExpandableAgents(int numAgents, int numObjects, bool truncateTreeSearch) const
    {
        std::vector<int> allAgents;
        allAgents.reserve(numAgents);
        for (int agent = 0; agent < numAgents; ++agent)
        {
            allAgents.push_back(agent);
        }

        if (!truncateTreeSearch)
        {
            return allAgents;
        }

        const std::vector<int> agentsWithoutObject = getAgentWithoutObject(numAgents);
        const int remainingObjects = getObjectsNotAllocated(numObjects);

        if (remainingObjects <= 0)
        {
            return {};
        }

        if (agentsWithoutObject.empty())
        {
            return allAgents;
        }

        if (static_cast<int>(agentsWithoutObject.size()) > remainingObjects)
        {
            return {};
        }

        if (static_cast<int>(agentsWithoutObject.size()) == remainingObjects)
        {
            return agentsWithoutObject;
        }

        return allAgents;
    }

    /**
     * @brief Get the number of children that can still be generated from this node.
     * @return int The number of expandable children.
     */
    int getMaxChildrenCount(int numAgents, int numObjects, bool truncateTreeSearch) const
    {
        return static_cast<int>(getExpandableAgents(numAgents, numObjects, truncateTreeSearch).size());
    }

    /**
     * @brief Check whether the node should be treated as a leaf for expansion.
     * @return bool True if the node should not be expanded.
     */
    bool isLeafForExpansion(int numAgents, int numObjects, bool truncateTreeSearch) const
    {
        if (getObjectsNotAllocated(numObjects) <= 0)
        {
            return true;
        }

        if (!truncateTreeSearch)
        {
            return false;
        }

        return static_cast<int>(getAgentWithoutObject(numAgents).size()) > getObjectsNotAllocated(numObjects);
    }

    /**
     * @brief Get the current allocation
     * @return const Allocation& The current allocation
     */
    const Allocation &getCurrentAllocation() const { return currentAllocation; };

    /**
     * @brief Get the number of visits
     * @return int The number of visits
     */
    int getVisits() const { return visits; };

    /**
     * @brief Get the index of the next child to expand
     * @return int The index of the next child to expand
     */
    int getChildrenIndex() const { return childrenIndex; };

    /**
     * @brief Increment the index of the next child to expand
     * @return void
     */
    void incrementChildrenIndex() { childrenIndex++; };
    /**
     * @brief Get the best allocation and score
     * @return const std::pair<Allocation, Score>& The best allocation and score
     */
    const std::pair<Allocation, Score> &getBestAllocation() const { return bestAllocation; };

    /**
     * @brief Set the height of the node in the tree
     * @return void
     */
    void setHeight(int height) { h = height; }

    /**
     * @brief Get the height of the node in the tree
     * @return int The height of the node in the tree
     */
    int getHeight() const { return h; };

    /**
     * @brief Get the children of the node
     * @return const std::vector<Node>& The children of the node
     */
    const std::vector<Node> &getChildren() const { return children; };

    /**
     * @brief Get the children of the node
     * @return std::vector<Node>& The children of the node
     */
    std::vector<Node> &getChildren() { return children; };

    /**
     * @brief Increment the number of visits
     * @return void
     */
    void incrementVisits() { visits++; };

    /**
     * @brief Set the number of visits
     * @param v The new number of visits to set
     * @return void
     */
    void setVisits(int v) { visits = v; };

    /**
     * @brief Update the best allocation and score
     * @param alloc The new best allocation and score
     * @return void
     */
    void updateBestAllocation(const std::pair<Allocation, Score> &alloc, bool verbose = false);

    /** @brief Expands a node by generating one of its unvisited children
     *  @param node The node to expand
     *  @return The expanded node
     */
    Node *extend(int numAgents, int numObjects, bool truncateTreeSearch, bool verbose)
    {
        // Generate a new child node by creating a new allocation based on the current allocation and modifying it
        int indexToModify = this->getHeight();

        if (indexToModify < 0 || indexToModify >= static_cast<int>(numObjects))
        {
            return nullptr;
        }

        const std::vector<int> expandableAgents = getExpandableAgents(numAgents, numObjects, truncateTreeSearch);
        if (expandableAgents.empty() || getChildrenIndex() >= static_cast<int>(expandableAgents.size()))
        {
            return nullptr;
        }

        // Get allocation and modify in-place to avoid copies
        std::vector<int> allocVec = this->getCurrentAllocation().getAllocation();

        allocVec[indexToModify] = expandableAgents[this->getChildrenIndex()];
        this->incrementChildrenIndex();

        // Use move semantics to avoid copying the allocation vector
        children.emplace_back(Allocation(numAgents, std::move(allocVec), verbose), this->getHeight() + 1, verbose);
        return &children.back();
    };

    /**
     * @brief Print debugging information about the node and its best allocation
     */
    void debug_print() const
    {
        std::cout << "Node at height " << h << " with allocation: ";
        const std::vector<int> &allocVec = currentAllocation.getAllocation();
        for (int object : allocVec)
        {
            std::cout << object << " ";
        }
        std::cout << "with score: " << bestAllocation.second.getScore() << std::endl;
        // Show best allocation and score for debugging purposes
        std::cout << "Best allocation (size : " << bestAllocation.first.getAllocation().size() << "): ";
        const std::vector<int> &bestAllocVec = bestAllocation.first.getAllocation();
        for (int object : bestAllocVec)
        {
            std::cout << object << " ";
        }
        std::cout << "with score: " << bestAllocation.second.getScore() << std::endl;
    }
};

#endif // NODE_HPP