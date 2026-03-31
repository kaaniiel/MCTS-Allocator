#include "mcts/MCTS.hpp"
#include "mcts/UCB.hpp"
#include "metrics/Utility.hpp"
#include "indicators/progress_bar.hpp"

#include <cmath>
#include <iostream>
#include <atomic>
#include <random>

#include "omp.h"
namespace
{

    // 1. Setup the beautiful progress bar
    indicators::ProgressBar createProgressBar(const std::string &postfix_text)
    {
        return indicators::ProgressBar{
            indicators::option::BarWidth{50},
            // 1. Remove the brackets
            indicators::option::Start{""},
            indicators::option::End{""},
            // 2. Use solid blocks for the filled part
            indicators::option::Fill{"\u2588"},
            indicators::option::Lead{"\u2588"},
            // indicators::option::Fill{"#"},
            // indicators::option::Lead{"#"},
            //  3. Use dotted blocks for the empty part
            indicators::option::Remainder{"\u2591"},
            // indicators::option::Remainder{"-"},
            //  4. Change color to Cyan
            indicators::option::ForegroundColor{indicators::Color::cyan},
            indicators::option::PostfixText{postfix_text},
            indicators::option::ShowPercentage{true},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true},
            indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}};
    }

    // 2. Thread-safe function to update the bar
    void updateProgress(indicators::ProgressBar &bar, std::atomic<int> &counter, int budget)
    {
        // Thread-safe incrementation
        int current = ++counter;

        // UI Optimization: Only draw the bar every 50 iterations, or at the very end
        if (current % 50 == 0 || current == budget)
        {

            {
                // FIXED: Calculate as double, then explicitly cast to size_t to avoid C4244 warnings
                size_t progress = static_cast<size_t>((static_cast<double>(current) / budget) * 100.0);
                bar.set_progress(progress);
            }
        }
    }

} // End of anonymous namespace

template <typename T>
void MCTS<T>::run(const int budget)
{
    auto bar = createProgressBar("Running MCTS...");
    std::atomic<int> completedIterations{0};

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

        updateProgress(bar, completedIterations, budget);
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
        // On utilise le générateur moderne du C++11, propre à chaque thread
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, currentAlloc.getNumAgents() - 1);

        // Génération sans aucun blocage
        int randomAgent = dist(rng);
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
    return std::make_pair(currentAlloc, Score(evalFunction(preferences, currentAlloc, this->getVerbose())));
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