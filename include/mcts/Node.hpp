#ifndef NODE_HPP
#define NODE_HPP

#include "Allocation.hpp"
#include "Score.hpp"

class Node
{
private:
    Allocation currentAllocation;
    int visits;
    std::pair<Allocation, Score> bestAllocation;
    int h; // height of the node in the tree

public:
    Node() : visits(0), h(0) {};
    Node(const Allocation &alloc) : currentAllocation(alloc), visits(0), h(0) {};
    Node(const Allocation &alloc, int height) : currentAllocation(alloc), visits(0), h(height) {};

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
     * @brief Get the height of the node in the tree
     * @return int The height of the node in the tree
     */
    int getHeight() const { return h; };

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