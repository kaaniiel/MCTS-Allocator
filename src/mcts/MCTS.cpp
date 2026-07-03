#include "mcts/MCTS.hpp"
#include "mcts/UCB.hpp"
#include "metrics/Utility.hpp"
#include "indicators/progress_bar.hpp"

#include <cmath>
#include <iostream>
#include <numeric>
#include <atomic>
#include <random>
#include <thread>
#include <functional>

#include "omp.h"
namespace
{

    /**
     * @brief Create a progress bar with a specific postfix text.
     * @param postfix_text Text to display after the progress bar.
     * @return A unique pointer to the configured ProgressBar.
     */
    std::unique_ptr<indicators::ProgressBar> createProgressBar(const std::string &postfix_text)
    {
        return std::make_unique<indicators::ProgressBar>(
            indicators::option::BarWidth{50},
            // 1. Remove the brackets
            indicators::option::Start{""},
            indicators::option::End{""},
            // 2. Use solid blocks for the filled part
            indicators::option::Fill{"\u2588"},
            indicators::option::Lead{"\u2588"},
            // 3. Use dotted blocks for the empty part
            indicators::option::Remainder{"\u2591"},
            // 4. Change color to Cyan
            indicators::option::ForegroundColor{indicators::Color::cyan},
            indicators::option::PostfixText{postfix_text},
            indicators::option::ShowPercentage{true},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true},
            indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}});
    }
    /**
     * @brief Thread-safe function to update the progress bar based on iteration budget.
     * @param bar The progress bar to update.
     * @param counter Atomic counter of completed iterations.
     * @param budget Total number of iterations.
     */
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
    /**
     * @brief Update the progress bar based on elapsed time rather than iterations.
     * @param bar The progress bar to update.
     * @param iteration Current iteration count (used to throttle UI updates).
     * @param elapsedSeconds Elapsed time in seconds.
     * @param budgetSeconds Total time budget in seconds.
     */
    void updateTimeProgress(indicators::ProgressBar &bar, int iteration, double elapsedSeconds, int budgetSeconds)
    {
        // Optimisation UI : On ne rafraîchit que toutes les 50 itérations pour préserver les performances
        if (iteration % 50 == 0)
        {
            size_t progress = static_cast<size_t>((elapsedSeconds / budgetSeconds) * 100.0);
            if (progress > 100)
                progress = 100;

            // Met à jour le texte à droite de la barre avec les secondes écoulées
            bar.set_option(indicators::option::PostfixText{
                "Running MCTS... [" + std::to_string(static_cast<int>(elapsedSeconds)) + "s / " + std::to_string(budgetSeconds) + "s]"});

            bar.set_progress(progress);
        }
    }
} // End of anonymous namespace

#include <memory>

template <typename T>
void MCTS<T>::run(const int budget, const double timeBudget, bool showProgress)
{
    if (workWithTimeBudget)
    {
        runWithTimeBudget(timeBudget, showProgress);
    }
    else
    {
        classicRun(budget, showProgress);
    }
}

template <typename T>
void MCTS<T>::classicRun(const int budget, bool showProgress)
{
    std::unique_ptr<indicators::ProgressBar> bar;
    if (showProgress)
    {
        bar = createProgressBar("Running MCTS...");
    }
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
        Node *simulationNode = (childNode != nullptr) ? childNode : node;
        if (simulationNode != nullptr)
        {
            if (childNode != nullptr && this->getVerbose())
            {
                for (int object : childNode->getCurrentAllocation().getAllocation())
                {
                    std::cout << object << " ";
                }
                std::cout << std::endl;
            }
            if (childNode != nullptr)
            {
                nodeStack.push(childNode);
            }
            /*simulation*/
            std::pair<Allocation, Score> reward = simulate(*simulationNode);
            /*backpropagation*/
            backpropagate(nodeStack, reward);
        }
        budgetCounter++; // Always increment to avoid infinite loop

        if (showProgress && bar)
        {
            updateProgress(*bar, completedIterations, budget);
        }
    }
}

template <typename T>
void MCTS<T>::runWithTimeBudget(const double timeBudget, bool showProgress)
{
    std::unique_ptr<indicators::ProgressBar> bar;
    if (showProgress)
    {
        bar = createProgressBar("Running MCTS with time budget...");
    }

    // 1. Démarrer le chrono au format haute résolution
    auto startTime = std::chrono::steady_clock::now();

    // 2. Convertir le budget en millisecondes (en long long pour éviter l'overflow)
    const long long budgetMillis = timeBudget * 1000LL;

    while (true)
    {
        nodeStack = std::stack<Node *>();

        // --- Selection ---
        Node *node = selectNode(&root, &nodeStack);

        // --- Expansion ---
        Node *childNode = node->extend();

        Node *simulationNode = (childNode != nullptr) ? childNode : node;
        if (simulationNode != nullptr)
        {
            if (childNode != nullptr)
            {
                nodeStack.push(childNode);
            }
            // --- Simulation ---
            std::pair<Allocation, Score> reward = simulate(*simulationNode);
            // --- Backpropagation ---
            backpropagate(nodeStack, reward);
        }

        this->budgetCounter++;

        // 3. Mesurer le temps écoulé en MILLISECONDES
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();

        // 4. Condition d'arrêt précise
        if (elapsedMillis >= budgetMillis)
        {
            if (showProgress && bar)
            {
                bar->set_option(indicators::option::PostfixText{
                    "Running MCTS... [" + std::to_string(timeBudget) + "s / " + std::to_string(timeBudget) + "s]"});
                bar->set_progress(100);
            }
            break;
        }

        // 5. Mise à jour de la barre basée sur le temps réel
        if (showProgress && bar)
        {
            // UI Optimization : On rafraîchit seulement toutes les 1000 itérations !
            // Cela empêche la console de brider la vitesse de ton algorithme.
            if (this->budgetCounter % 1000 == 0)
            {
                double elapsedSecondsDouble = elapsedMillis / 1000.0;
                updateTimeProgress(*bar, this->budgetCounter, elapsedSecondsDouble, timeBudget);
            }
        }
    }
}
template <typename T>
Node *MCTS<T>::selectNode(Node *node, std::stack<Node *> *nodeStack)
{
    nodeStack->push(node);
    Node *currentNode = node;
    while (!currentNode->isLeafForExpansion() && currentNode->getChildren().size() >= static_cast<std::size_t>(currentNode->getMaxChildrenCount()))
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

    // Simulate down the tree with temporary allocations (not adding to tree)
    int currentHeight = node.getHeight();
    int numAgents = currentAlloc.getNumAgents();
    thread_local static std::mt19937_64 rng64(std::random_device{}() ^ (std::hash<std::thread::id>{}(std::this_thread::get_id())));
    thread_local static std::uniform_real_distribution<double> dist(0.0, 1.0);
    while (currentHeight < currentAlloc.getNumObjects())
    {
        std::vector<int> nextAllocVec = currentAlloc.getAllocation();
        const bool truncateTree = node.getTruncateTreeSearch();

        std::vector<bool> hasObject(numAgents, false);
        for (int object = 0; object < currentHeight; ++object)
        {
            const int assignedAgent = nextAllocVec[object];
            if (assignedAgent >= 0 && assignedAgent < numAgents)
            {
                hasObject[assignedAgent] = true;
            }
        }

        std::vector<int> agentsWithoutObject;
        agentsWithoutObject.reserve(numAgents);
        for (int a = 0; a < numAgents; ++a)
        {
            if (!hasObject[a])
            {
                agentsWithoutObject.push_back(a);
            }
        }

        const int remainingObjects = currentAlloc.getNumObjects() - currentHeight;
        double totalPossibleSubtreeLeaves = 0;
        if (config.monitoringCuts)
        {
            totalPossibleSubtreeLeaves = std::pow(numAgents, remainingObjects);
        }

        if (truncateTree && static_cast<int>(agentsWithoutObject.size()) > remainingObjects)
        {
            if (config.monitoringCuts)
            {
                monitoringCuts += static_cast<unsigned long long>(totalPossibleSubtreeLeaves);
            }
            return std::make_pair(currentAlloc, Score(-std::numeric_limits<double>::infinity()));
        }

        // ---------------------------------------------------------
        // 2. Coupe d'entonnoir : Évaluation de la contrainte forcée
        // ---------------------------------------------------------
        // On stocke le booléen pour s'en servir dans les heuristiques juste après
        bool isForcedPath = truncateTree && !agentsWithoutObject.empty() &&
                            static_cast<int>(agentsWithoutObject.size()) == remainingObjects;

        if (isForcedPath && config.monitoringCuts)
        {
            unsigned long long keptPaths = factorial(remainingObjects);
            // On comptabilise l'espace mathématique éliminé
            monitoringCuts += static_cast<unsigned long long>(totalPossibleSubtreeLeaves) - keptPaths;
        }

        // ---------------------------------------------------------
        // 3. Choix de l'agent (Aléatoire ou Heuristique)
        // ---------------------------------------------------------

        double randomValue = dist(rng64);
        int agent = -1;
        ratioRandom = politic->get_ratio_limit(currentAlloc);
        if (randomValue < ratioRandom)
        {
            // --- Logique Aléatoire ---
            if (isForcedPath)
            {
                const int selectedIndex = static_cast<int>(dist(rng64) * agentsWithoutObject.size());
                const int clampedIndex = (selectedIndex >= static_cast<int>(agentsWithoutObject.size()))
                                             ? static_cast<int>(agentsWithoutObject.size()) - 1
                                             : selectedIndex;
                agent = agentsWithoutObject[clampedIndex];
            }
            else
            {
                agent = static_cast<int>(dist(rng64) * numAgents);
                agent = (agent >= numAgents) ? numAgents - 1 : agent;
            }
        }
        else
        {
            // --- Logique Heuristique ---
            // On utilise notre booléen isForcedPath pour déterminer les candidats !
            const std::vector<int> &candidateAgents = isForcedPath ? agentsWithoutObject : std::vector<int>{};

            int objectIndex = currentHeight;
            double bestPref = -std::numeric_limits<double>::infinity();

            if (!candidateAgents.empty())
            {
                for (int a : candidateAgents)
                {
                    double pref = preferences.getPreference(a, objectIndex);
                    if (pref > bestPref)
                    {
                        bestPref = pref;
                        agent = a;
                    }
                }
            }
            else
            {
                for (int a = 0; a < numAgents; ++a)
                {
                    double pref = preferences.getPreference(a, objectIndex);
                    if (pref > bestPref)
                    {
                        bestPref = pref;
                        agent = a;
                    }
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
    Score score;
    if (addMetricsToUtility)
    {
        score = Score(Utility<T>::addMetrics2Utility(preferences, currentAlloc, evalFunction, this->config, this->getVerbose()));
    }
    else
    {
        score = Score(evalFunction(preferences, currentAlloc, this->getVerbose()));
    }
    return std::make_pair(currentAlloc, score);
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
            iterTrackerBestSolution = 1;                  // Reset the timer if a better solution is found
        }
        else
        {
            iterTrackerBestSolution++; // Increment the timer if no better solution is found
        }
    }
    return bestAlloc;
}

template class MCTS<int>;