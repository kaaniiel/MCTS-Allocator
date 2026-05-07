#ifndef UTILITY_HPP
#define UTILITY_HPP
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "../mcts/Preferences.hpp"
#include "../mcts/Allocation.hpp"

template <typename T>
class Utility
{
public:
    /**
     * @brief Calculate the utility value for a given allocation. We multiply the utilities of each agent together to get the overall utility of the allocation. This encourages allocations that are good for all agents rather than just one.
     * @param prefs The preferences for the agents and objects
     * @param alloc The allocation for which to calculate the utility
     * @return double The calculated utility value
     */
    static double calculateUtilityMul(const Preferences<T> &prefs, const Allocation &alloc, const bool verbose = false)
    {
        const std::vector<int> &allocation = alloc.getAllocation();
        int numAgents = alloc.getNumAgents();
        int numObjects = alloc.getNumObjects();

        // Use thread_local to avoid repeated memory allocation across MCTS simulations
        thread_local std::vector<double> agentUtilities;
        if (agentUtilities.size() < static_cast<size_t>(numAgents))
        {
            agentUtilities.resize(numAgents, 0.0);
        }
        else
        {
            std::fill(agentUtilities.begin(), agentUtilities.begin() + numAgents, 0.0);
        }

        for (int object = 0; object < numObjects; ++object)
        {
            int agent = allocation[object];
            if (agent >= 0 && agent < numAgents)
            { // Valid agent assignment
                agentUtilities[agent] += prefs.getPreference(agent, object);
            }
        }

        // Keep scoring consistent with the solver objective: sum(log(eps + u_i)).
        const double eps = 1e-6;
        double totalUtility = 0.0;
        for (int agent = 0; agent < numAgents; ++agent)
        {
            if (verbose)
            {
                std::cout << "Agent " << agent << " utility: " << agentUtilities[agent] << std::endl;
            }
            totalUtility += std::log(agentUtilities[agent] + eps);
        }
        return totalUtility;
    }
};
#endif // UTILITY_HPP