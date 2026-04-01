#include "mcts/MCTS.hpp"
#include "mcts/UCB.hpp"
#include "metrics/Utility.hpp"
#include "indicators/progress_bar.hpp"

#include <cmath>
#include <iostream>
#include <atomic>
#include <random>
#include <thread>
#include <functional>

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
    // Simulate initial score at 0.0
    Score bestSimulValue = Score(0.0);

    if (this->getVerbose())
    {
        std::cout << "[Simulate] Starting from height " << node.getHeight() << " with allocation: ";
        const std::vector<int> &allocVec = currentAlloc.getAllocation();
        for (int object : allocVec)
            std::cout << object << " ";
        std::cout << std::endl;
    }

    // Initialize MT19937_64 (64-bit Mersenne Twister) once per thread
    // Better distribution than 32-bit version and slightly faster for random number generation
    thread_local static std::mt19937_64 rng64(std::random_device{}() ^ (std::hash<std::thread::id>{}(std::this_thread::get_id())));
    thread_local static std::uniform_real_distribution<double> dist(0.0, 1.0);

    // Simulate down the tree with temporary allocations (not adding to tree)
    int currentHeight = node.getHeight();
    int numAgents = currentAlloc.getNumAgents();

    while (currentHeight < currentAlloc.getNumObjects())
    {
        std::vector<int> nextAllocVec = currentAlloc.getAllocation();

        // Decide randomly whether to do a random simulation or a heuristic simulation based on the ratioRandomSimulation
        double randomValue = dist(rng64);
        int agent = -1;
        if (randomValue < ratioRandom)
        {
            // Perform a random simulation: assign a random agent to the current object

            // Faster random int generation: use real distribution [0,1) then multiply
            // This avoids the overhead of uniform_int_distribution for every call
            agent = static_cast<int>(dist(rng64) * numAgents);
            // Clamp to valid range just in case of floating point edge cases
            agent = (agent >= numAgents) ? numAgents - 1 : agent;
        }
        else
        {
            // Perform a heuristic simulation: assign the agent with the highest preference for the current object
            int objectIndex = currentHeight;
            double bestPref = -std::numeric_limits<double>::infinity();
            for (int a = 0; a < numAgents; a++)
            {
                double pref = preferences.getPreference(a, objectIndex);
                if (pref > bestPref)
                {
                    bestPref = pref;
                    agent = a;
                }
            }
        }
        nextAllocVec[currentHeight] = agent;
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
        if (currentNode->getBestAllocation().second.getScore() > bestAlloc.second.getScore())
        {
            bestAlloc = currentNode->getBestAllocation(); // Update the best allocation and score if the current node's best allocation has a better score than the current best allocation
        }
    }
    return bestAlloc;
}

template class MCTS<int>;