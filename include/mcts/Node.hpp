#ifndef NODE_HPP
#define NODE_HPP

#include <iostream>
#include <vector>
#include <random>
#include <ctime>

#include "Allocation.hpp"
#include "Score.hpp"

class Node
{
private:
    int numObjects; // Number of objects to allocate, needed for generating children
    int numAgents;  // Number of agents, needed for generating children
    int visits;
    int h;             // height of the node in the tree
    int childrenIndex; // index of the next child to expand
    Allocation currentAllocation;
    std::pair<Allocation, Score> bestAllocation;
    std::vector<Node> children;
    bool verbose;

public:
    Node() : numObjects(0),
             numAgents(0),
             visits(0),
             h(0),
             childrenIndex(0),
             currentAllocation(Allocation()),
             bestAllocation(Allocation(), Score()),
             verbose(false) {};
    Node(const int numAgents, const int numObjects, const bool verbose = false) : numObjects(numObjects),
                                                                                  numAgents(numAgents),
                                                                                  visits(0),
                                                                                  h(0),
                                                                                  childrenIndex(0),
                                                                                  currentAllocation(Allocation(numAgents, numObjects, verbose)),
                                                                                  bestAllocation(Allocation(numAgents, numObjects, verbose), Score(std::vector<double>{0.0}, verbose)),
                                                                                  verbose(verbose)

    {
        bestAllocation.first.setAllocation(currentAllocation.getAllocation());
        bestAllocation.second.setScores(std::vector<double>{0.0});
    };

    Node(const Node &other) : numObjects(other.numObjects),
                              numAgents(other.numAgents),
                              visits(other.visits),
                              h(other.h),
                              childrenIndex(0),
                              currentAllocation(other.currentAllocation),
                              bestAllocation(other.bestAllocation),
                              verbose(other.verbose) {};

    Node(const Allocation &alloc) : numObjects(alloc.getNumObjects()),
                                    numAgents(alloc.getNumAgents()),
                                    visits(0),
                                    h(0),
                                    childrenIndex(0),
                                    currentAllocation(alloc),
                                    bestAllocation(alloc, Score(std::vector<double>{0.0}, alloc.getVerbose())),
                                    verbose(alloc.getVerbose()) {};

    Node(const Allocation &alloc, int height, const bool verbose = false) : numObjects(alloc.getNumObjects()),
                                                                            numAgents(alloc.getNumAgents()),
                                                                            visits(0),
                                                                            h(height),
                                                                            childrenIndex(0),
                                                                            currentAllocation(alloc),
                                                                            bestAllocation(alloc, Score(std::vector<double>{0.0}, verbose)),
                                                                            verbose(verbose) {};

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
     * @return int The height of the node in the tree
     */
    int setHeight(int height) { h = height; };

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
     * @brief Update the best allocation and score
     * @param alloc The new best allocation and score
     * @return void
     */
    void updateBestAllocation(const std::pair<Allocation, Score> &alloc);

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

    /** @brief Expands a node by generating one of its unvisited children
     *  @param node The node to expand
     *  @return The expanded node
     */
    Node *extend()
    {
        // Generate a new child node by creating a new allocation based on the current allocation and modifying it
        Allocation newAlloc = this->getCurrentAllocation();
        std::vector<int> allocVec = newAlloc.getAllocation();

        int indexToModify = this->getHeight();

        int numAgents = this->getNumAgents();
        if (indexToModify < 0 || indexToModify >= static_cast<int>(allocVec.size()))
        {
            return nullptr;
        }

        if (allocVec[indexToModify] + (this->getChildrenIndex() + 1) >= numAgents)
        {
            // If the modified allocation exceeds the number of agents, we cannot create a new child node
            return nullptr;
        }
        allocVec[indexToModify] = (allocVec[indexToModify] + (this->getChildrenIndex() + 1)); // Modify the allocation for the current index
        this->incrementChildrenIndex();                                                       // Increment the index of the next child to expand

        newAlloc.setAllocation(allocVec);
        children.emplace_back(newAlloc, this->getHeight() + 1, this->getVerbose()); // Persist child in the tree
        return &children.back();
    };

    void debug_print() const
    {
        std::cout << "Node at height " << h << " with allocation: ";
        const std::vector<int> &allocVec = currentAllocation.getAllocation();
        for (int object : allocVec)
        {
            std::cout << object << " ";
        }
        std::cout << "with score: " << bestAllocation.second.getScores()[0] << std::endl;
        // Show best allocation and score for debugging purposes
        std::cout << "Best allocation (size : " << bestAllocation.first.getAllocation().size() << "): ";
        const std::vector<int> &bestAllocVec = bestAllocation.first.getAllocation();
        for (int object : bestAllocVec)
        {
            std::cout << object << " ";
        }
        std::cout << "with score: " << bestAllocation.second.getScores()[0] << std::endl;
    }
};

#endif // NODE_HPP