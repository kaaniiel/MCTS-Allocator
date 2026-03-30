#include "mcts/MCTS.hpp"
#include "mcts/UCB.hpp"
#include "metrics/Utility.hpp"

#include <cmath>
#include <iostream>
#include "omp.h"

template <typename T>
void MCTS<T>::run(const int budget)
{
    int budgetCounter = 0;
    while (budgetCounter < budget)
    {
        nodeStack = std::stack<Node *>(); // Clear the stack at the beginning of each run
        /*Selection*/
        if (this->getVerbose())
        {
            std::cout << "Selecting node..." << std::endl;
        }
        Node *node = selectNode(&root, &nodeStack);
        /*expansion*/
        if (this->getVerbose())
        {
            std::cout << "Expanding node..." << std::endl;
        }
        Node *childNode = node->extend();
        if (this->getVerbose())
        {
            std::cout << "Budget counter: " << budgetCounter << std::endl;
        }
        if (childNode != nullptr)
        {
            if (this->getVerbose())
            {
                for (int object : childNode->getCurrentAllocation().getAllocation())
                {
                    std::cout << object << " ";
                }
                std::cout << std::endl;
            }
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
    int max_system_threads = omp_get_max_threads();
    int actual_threads = 1; // Default fallback

    if (numThreads == -1 || numThreads > max_system_threads)
    {
        // If the user requested -1 (all threads) or asked for more threads
        // than the machine can physically handle, we cap it to the maximum available.
        actual_threads = max_system_threads;
    }
    else if (numThreads > 0)
    {
        // Use the exact requested number of threads
        actual_threads = numThreads;
    }

    // Optional: Good practice to log this so the user knows what's happening
    // std::cout << "[MCTS] Launching parallel execution with " << actual_threads << " threads.\n";

    omp_set_num_threads(actual_threads);
    if (verbose)
    {
        std::cout << "[MCTS] Launching parallel execution with " << actual_threads << " threads.\n";
    }
#pragma omp parallel
    {
#pragma omp single
        {
            int budgetCounter = 0;
            while (budgetCounter < budget)
            {
                nodeStack = std::stack<Node *>(); // Clear the stack at the beginning of each run
                /*Selection*/
                if (verbose)
                {
                    std::cout << "[MCTS] Selection phase..." << std::endl;
                }
                Node *node = selectNode(&root, &nodeStack);
                /*expansion*/
                if (verbose)
                {
                    std::cout << "[MCTS] Expansion phase..." << std::endl;
                }

                for (int i = 0; i < node->getNumAgents(); i++)
                {
                    if (budgetCounter >= budget)
                        break;
                    budgetCounter++;
                    Node *childNode = node->extend();
                    if (childNode != nullptr)
                    {
                        if (this->getVerbose())
                        {
                            std::cout << "[MCTS] Budget counter: " << budgetCounter << ", extended node with allocation: ";
                            for (int object : childNode->getCurrentAllocation().getAllocation())
                            {
                                std::cout << object << " ";
                            }
                            std::cout << std::endl;
                        }
                        std::stack<Node *> localStack = nodeStack;
                        localStack.push(childNode);
#pragma omp task firstprivate(localStack, childNode)
                        {
                            if (this->getVerbose())
                            {
                                std::cout << "[MCTS] Running simulation and backpropagation..." << std::endl;
                            }
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
    // Algorithm 4: Create TEMPORARY allocations during simulation, not persistent nodes
    Allocation currentAlloc = node.getCurrentAllocation();
    Score bestSimulValue = Score(std::vector<double>{0.0});

    if (this->getVerbose())
    {
        std::cout << "[Simulate] Starting from height " << node.getHeight() << " with allocation: ";
        const std::vector<int> &allocVec = currentAlloc.getAllocation();
        for (int object : allocVec)
            std::cout << object << " ";
        std::cout << std::endl;
    }

    // Simulate down the tree with temporary allocations (not adding to tree)
    int currentHeight = node.getHeight();
    while (currentHeight < currentAlloc.getNumObjects())
    {
        std::vector<int> nextAllocVec = currentAlloc.getAllocation();

        // Randomly assign the next object to an agent
        int randomAgent = rand() % currentAlloc.getNumAgents();
        nextAllocVec[currentHeight] = randomAgent;

        currentAlloc.setAllocation(nextAllocVec);
        currentHeight++;

        if (this->getVerbose())
        {
            std::cout << "[Simulate] Height " << currentHeight << " allocation: ";
            for (int obj : nextAllocVec)
                std::cout << obj << " ";
            std::cout << std::endl;
        }
    }

    // Calculate value for the final allocation
    // For now, return empty score (will be calculated by caller if needed)
    return std::make_pair(currentAlloc, Score(std::vector<double>{0.0}));
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
        currentNode->incrementVisits();            // Increment the visit count for the current node
        currentNode->updateBestAllocation(reward); // Update the best allocation and score for the current node based on the reward from the simulation
        if (currentNode->getBestAllocation().second.getScores()[0] > bestAlloc.second.getScores()[0])
        {
            bestAlloc = currentNode->getBestAllocation(); // Update the best allocation and score if the current node's best allocation has a better score than the current best allocation
        }
    }
    return bestAlloc;
}

template class MCTS<int>;