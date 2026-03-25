#include "mcts/Node.hpp"

void Node::updateBestAllocation(const std::pair<Allocation, Score> &alloc)
{
    // Update the best allocation and score if the new score is better than the current best score
    if (bestAllocation.first.getAllocation().empty() || alloc.second.getScores()[0] > bestAllocation.second.getScores()[0])
    {
        bestAllocation = alloc;
    }
}