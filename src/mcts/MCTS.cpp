#include "mcts/MCTS.hpp"
#include "mcts/UCB.hpp"
#include "metrics/Utility.hpp"

#include <cmath>
#include <iostream>

template <typename T>
void MCTS<T>::run(const int budget)
{
    int budgetCounter = 0;
    while (budgetCounter < budget)
    {
        nodeStack = std::stack<Node *>(); // Clear the stack at the beginning of each run
        /*Selection*/
        Node *node = selectNode(&root, &nodeStack);
        /*expansion*/
        Node *childNode = node->extend();
        std::cout << "Budget counter: " << budgetCounter << std::endl;
        if (childNode != nullptr)
        {
            std::cout << "Child Node: " << " ";
            for (int object : childNode->getCurrentAllocation().getAllocation())
            {
                std::cout << object << " ";
            }
            std::cout << std::endl;
            nodeStack.push(childNode);
            /*simulation*/
            std::pair<Allocation, Score> reward = simulate(*childNode);
            /*backpropagation*/
            backpropagate(nodeStack, reward);
        }
        budgetCounter++; // Always increment to avoid infinite loop
    }
}

void MCTS<int>::parallelRun(const int budget)
{
#pragma omp parallel
    {
#pragma omp single
        {
            int budgetCounter = 0;
            while (budgetCounter < budget)
            {
                nodeStack = std::stack<Node *>(); // Clear the stack at the beginning of each run
                /*Selection*/
                Node *node = selectNode(&root, &nodeStack);
                /*expansion*/

                for (int i = 0; i < node->getNumAgents(); i++)
                {
                    if (budgetCounter >= budget)
                        break;
                    budgetCounter++;
                    Node *childNode = node->extend();
                    if (childNode != nullptr)
                    {
                        std::stack<Node *> localStack = nodeStack;
                        localStack.push(childNode);
#pragma omp task firstprivate(localStack, childNode)
                        {
                            /*simulation*/
                            std::pair<Allocation, Score> reward = simulate(*childNode);
                            /*backpropagation*/
                            backpropagate(localStack, reward);
                        }
                    }
                }
            }
        }
    }
}

template <typename T>
Node *MCTS<T>::selectNode(Node *node, std::stack<Node *> *nodeStack)
{
    nodeStack->push(node);
    Node *currentNode = node;
    while (currentNode->getChildren().size() >= static_cast<std::size_t>(currentNode->getNumAgents()))
    {
        currentNode = UCB::selectBestChild(currentNode, std::sqrt(2.0)); // Use an exploration parameter of sqrt(2) for UCB
        if (currentNode == nullptr)
        {
            break;
        }
        nodeStack->push(currentNode);
    }
    return currentNode;
}

template <typename T>
std::pair<Allocation, Score> MCTS<T>::simulate(Node &node)
{
    Allocation maxAllocation;
    Score maxScore;
    Node *currentNode = &node;
    while (currentNode->getHeight() < currentNode->getNumObjects())
    {
        Node *childNode = currentNode->extendsRandom();
        if (childNode == nullptr)
        {
            break; // If we cannot extend further, break the loop
        }
        currentNode = childNode;
        std::pair<Allocation, Score> currentAllocScore = std::make_pair(childNode->getCurrentAllocation(), Score(std::vector<double>{Utility<T>::calculateUtilityMul(preferences, childNode->getCurrentAllocation())}));
        if (currentAllocScore.second.getScores()[0] > maxScore.getScores()[0])
        {
            maxScore = currentAllocScore.second;
            maxAllocation = currentAllocScore.first;
        }
    }

    return std::make_pair(maxAllocation, maxScore); // Return the best allocation and score found during the simulation
}

template <typename T>
std::pair<Allocation, Score> MCTS<T>::backpropagate(std::stack<Node *> &nodeStack, const std::pair<Allocation, Score> &reward)
{
    // Backpropagate the reward obtained from a simulation up the tree and return the best allocation and score found during backpropagation
    std::pair<Allocation, Score> bestAlloc = reward; // Initialize the best allocation and score with the reward from the simulation
    while (!nodeStack.empty())
    {
        Node *currentNode = nodeStack.top();
        nodeStack.pop();

        currentNode->updateBestAllocation(reward); // Update the best allocation and score for the current node based on the reward from the simulation
        if (currentNode->getBestAllocation().second.getScores()[0] > bestAlloc.second.getScores()[0])
        {
            bestAlloc = currentNode->getBestAllocation(); // Update the best allocation and score if the current node's best allocation has a better score than the current best allocation
        }
    }
    return bestAlloc;
}

template class MCTS<int>;