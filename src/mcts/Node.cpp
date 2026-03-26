#include "mcts/Node.hpp"
#include <optional>
void Node::updateBestAllocation(const std::pair<Allocation, Score> &alloc)
{
    // Update the best allocation and score if the new score is better than the current best score
    if (bestAllocation.first.getAllocation().empty() || alloc.second.getScores()[0] > bestAllocation.second.getScores()[0])
    {
        bestAllocation = alloc;
    }
}

std::optional<Node> Node::extendNode(Node &node)
{
    // Generate a new child node by creating a new allocation based on the current allocation and modifying it
    Allocation newAlloc = node.getCurrentAllocation();
    std::vector<int> allocVec = newAlloc.getAllocation();

    int indexToModify = node.getHeight();

    int numAgents = node.getNumAgents();
    if (allocVec[indexToModify] + (node.getChildrenIndex() + 1) >= numAgents)
    {
        // If the modified allocation exceeds the number of agents, we cannot create a new child node
        return std::nullopt; // Return an empty optional to indicate that no more children can be expanded
    }
    allocVec[indexToModify] = (allocVec[indexToModify] + (node.getChildrenIndex() + 1)); // Modify the allocation for the current index
    node.incrementChildrenIndex();                                                       // Increment the index of the next child to expand

    newAlloc.setAllocation(allocVec);
    Node childNode(newAlloc, node.getHeight() + 1); // Create a new child node with the modified allocation and incremented height

    return childNode;
}