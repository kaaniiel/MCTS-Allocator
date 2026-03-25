#ifndef NODE_HPP
#define NODE_HPP

#include "Allocation.hpp"
#include "Score.hpp"

class Node
{
private:
    int numObjects; // Number of objects to allocate, needed for generating children
    int numAgents;  // Number of agents, needed for generating children
    int visits;
    int h; // height of the node in the tree
    Allocation currentAllocation;
    std::pair<Allocation, Score> bestAllocation;
    std::vector<Node> children;

public:
    Node() : numObjects(0),
             numAgents(0),
             visits(0),
             h(0),
             currentAllocation(Allocation()) {};
    Node(const int numAgents, const int numObjects) : numObjects(numObjects),
                                                      numAgents(numAgents),
                                                      visits(0),
                                                      h(0),
                                                      currentAllocation(Allocation(numAgents, numObjects)) {};

    Node(const Node &other) : numObjects(other.numObjects),
                              numAgents(other.numAgents),
                              visits(other.visits),
                              h(other.h),
                              currentAllocation(other.currentAllocation),
                              bestAllocation(other.bestAllocation) {};

    Node(const Allocation &alloc) : numObjects(alloc.getNumObjects()),
                                    numAgents(alloc.getNumAgents()),
                                    visits(0),
                                    h(0),
                                    currentAllocation(alloc) {};

    Node(const Allocation &alloc, int height) : numObjects(alloc.getNumObjects()),
                                                numAgents(alloc.getNumAgents()),
                                                visits(0),
                                                h(height),
                                                currentAllocation(alloc) {};

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
};

#endif // NODE_HPP