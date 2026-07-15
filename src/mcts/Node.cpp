#include "mcts/Node.hpp"
#include <optional>
#include <numeric>
#include <iostream>

void Node::updateBestAllocation(const std::pair<Allocation, Score> &alloc, bool verbose)
{
    // Update best allocation if uninitialized, better score, or equal score.
    // Keeping equal-score updates avoids freezing on an early zero-score allocation.
    if (std::accumulate(bestAllocation.first.getAllocation().begin(), bestAllocation.first.getAllocation().end(), 0) == -static_cast<int>(bestAllocation.first.getAllocation().size()) || alloc.second.getScore() >= bestAllocation.second.getScore())
    {
        bestAllocation = alloc;
    }
    // show new best allocation and score for debugging purposes
    if (verbose)
    {
        std::cout << "Updated best allocation: ";
        const std::vector<int> &updatedBestAllocVec = bestAllocation.first.getAllocation();
        for (int object : updatedBestAllocVec)
        {
            std::cout << object << " ";
        }
        std::cout << "with score: " << bestAllocation.second.getScore() << std::endl;
        std::cout << "" << std::endl; // Print a newline for better readability in the debug output
    }
}